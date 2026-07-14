#include <TeensyMasterCore.h>
#include <unity.h>

using namespace Haptic;
namespace TM = Haptic::TeensyMaster;

static ModulePacket makePacket(uint8_t kind = MODULE_KIND_LOADCELL) {
  ModulePacket packet{};
  packet.protocolVersion = kProtocolVersion;
  packet.moduleKind = kind;
  packet.status = MODULE_STATUS_OK;
  packet.sequence = 42;
  packet.idAdc = 1234;
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    packet.payload[i] = static_cast<int16_t>(100 + i);
  }
  finalizePacket(packet);
  return packet;
}

void test_module_address_table_matches_expected_bus_addresses() {
  TEST_ASSERT_EQUAL_UINT8(3, TM::kNumModules);
  TEST_ASSERT_EQUAL_HEX8(0x20, TM::kModuleAddresses[0]);
  TEST_ASSERT_EQUAL_HEX8(0x21, TM::kModuleAddresses[1]);
  TEST_ASSERT_EQUAL_HEX8(0x22, TM::kModuleAddresses[2]);
}

void test_poll_interval_handles_normal_and_wrapped_millis() {
  TEST_ASSERT_FALSE(TM::isPollDue(109, 100));
  TEST_ASSERT_TRUE(TM::isPollDue(110, 100));
  TEST_ASSERT_TRUE(TM::isPollDue(4, UINT32_MAX - 5));
}

void test_decode_accepts_valid_packet_bytes() {
  ModulePacket source = makePacket(MODULE_KIND_ENCODER);
  ModulePacket decoded{};

  const auto result = TM::decodePacketBytes(
      reinterpret_cast<const uint8_t *>(&source), sizeof(source), decoded);

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TM::PacketDecodeResult::Ok),
                          static_cast<uint8_t>(result));
  TEST_ASSERT_EQUAL_UINT8(MODULE_KIND_ENCODER, decoded.moduleKind);
  TEST_ASSERT_EQUAL_UINT8(42, decoded.sequence);
  TEST_ASSERT_EQUAL_INT16(100, decoded.payload[0]);
  TEST_ASSERT_TRUE(validatePacket(decoded));
}

void test_decode_rejects_wrong_length() {
  ModulePacket source = makePacket();
  ModulePacket decoded{};

  const auto result = TM::decodePacketBytes(
      reinterpret_cast<const uint8_t *>(&source), sizeof(source) - 1, decoded);

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TM::PacketDecodeResult::WrongLength),
                          static_cast<uint8_t>(result));
}

void test_decode_rejects_wrong_protocol() {
  ModulePacket source = makePacket();
  source.protocolVersion = kProtocolVersion + 1;
  finalizePacket(source);
  ModulePacket decoded{};

  const auto result = TM::decodePacketBytes(
      reinterpret_cast<const uint8_t *>(&source), sizeof(source), decoded);

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TM::PacketDecodeResult::WrongProtocol),
                          static_cast<uint8_t>(result));
}

void test_decode_rejects_bad_checksum() {
  ModulePacket source = makePacket();
  source.payload[0] ^= 0x01;
  ModulePacket decoded{};

  const auto result = TM::decodePacketBytes(
      reinterpret_cast<const uint8_t *>(&source), sizeof(source), decoded);

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TM::PacketDecodeResult::BadChecksum),
                          static_cast<uint8_t>(result));
}

void test_scan_result_resets_state() {
  TM::ModuleState state{};
  state.active = true;
  state.missCount = 4;
  state.lastSuccessMs = 10;

  TM::recordScanResult(state, false, 500);

  TEST_ASSERT_FALSE(state.active);
  TEST_ASSERT_EQUAL_UINT8(0, state.missCount);
  TEST_ASSERT_EQUAL_UINT32(500, state.lastSuccessMs);
}

void test_read_success_marks_active_and_resets_misses() {
  TM::ModuleState state{};
  state.active = false;
  state.missCount = TM::kMissLimit;
  state.lastSuccessMs = 100;

  TM::recordReadSuccess(state, 250);

  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_EQUAL_UINT8(0, state.missCount);
  TEST_ASSERT_EQUAL_UINT32(250, state.lastSuccessMs);
}

void test_read_failure_increments_until_limit_without_timeout() {
  TM::ModuleState state{};
  state.active = true;
  state.lastSuccessMs = 1000;

  for (uint8_t i = 0; i < TM::kMissLimit; ++i) {
    TEST_ASSERT_FALSE(TM::recordReadFailure(state, 1000 + i));
  }

  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_EQUAL_UINT8(TM::kMissLimit, state.missCount);
  TEST_ASSERT_FALSE(TM::recordReadFailure(state, 1000 + TM::kModuleTimeoutMs - 1));
  TEST_ASSERT_EQUAL_UINT8(TM::kMissLimit, state.missCount);
}

void test_read_failure_deactivates_after_miss_limit_and_timeout() {
  TM::ModuleState state{};
  state.active = true;
  state.lastSuccessMs = 1000;

  for (uint8_t i = 0; i < TM::kMissLimit - 1; ++i) {
    TEST_ASSERT_FALSE(TM::recordReadFailure(state, 1200));
  }

  TEST_ASSERT_TRUE(TM::recordReadFailure(state, 1200));
  TEST_ASSERT_FALSE(state.active);
  TEST_ASSERT_EQUAL_UINT8(TM::kMissLimit, state.missCount);
}

void test_parse_loadcell_payload() {
  ModulePacket packet = makePacket(MODULE_KIND_LOADCELL);
  packet.payload[0] = -123;
  packet.payload[1] = 456;
  finalizePacket(packet);

  const TM::LoadcellReading reading = TM::parseLoadcellReading(packet);

  TEST_ASSERT_EQUAL_INT16(-123, reading.cellA);
  TEST_ASSERT_EQUAL_INT16(456, reading.cellB);
}

void test_parse_pressure_payload() {
  ModulePacket packet = makePacket(MODULE_KIND_PRESSURE);
  packet.payload[0] = 250;
  packet.payload[1] = 1106;
  packet.payload[2] = 166;
  finalizePacket(packet);

  const TM::PressureReading reading = TM::parsePressureReading(packet);

  TEST_ASSERT_EQUAL_INT16(250, reading.pressureCentikpa);
  TEST_ASSERT_EQUAL_INT16(1106, reading.rawAdc);
  TEST_ASSERT_EQUAL_INT16(166, reading.zeroAdc);
}

void test_parse_encoder_payload_reconstructs_signed_position() {
  ModulePacket packet = makePacket(MODULE_KIND_ENCODER);
  const int32_t position = -1234567;
  packet.payload[0] = static_cast<int16_t>(position & 0xFFFF);
  packet.payload[1] = static_cast<int16_t>((position >> 16) & 0xFFFF);
  packet.payload[2] = -321;
  packet.payload[3] = -1;
  finalizePacket(packet);

  const TM::EncoderReading reading = TM::parseEncoderReading(packet);

  TEST_ASSERT_EQUAL_INT32(position, reading.position);
  TEST_ASSERT_EQUAL_INT16(-321, reading.velocityTenthRpm);
  TEST_ASSERT_EQUAL_INT16(-1, reading.direction);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_module_address_table_matches_expected_bus_addresses);
  RUN_TEST(test_poll_interval_handles_normal_and_wrapped_millis);
  RUN_TEST(test_decode_accepts_valid_packet_bytes);
  RUN_TEST(test_decode_rejects_wrong_length);
  RUN_TEST(test_decode_rejects_wrong_protocol);
  RUN_TEST(test_decode_rejects_bad_checksum);
  RUN_TEST(test_scan_result_resets_state);
  RUN_TEST(test_read_success_marks_active_and_resets_misses);
  RUN_TEST(test_read_failure_increments_until_limit_without_timeout);
  RUN_TEST(test_read_failure_deactivates_after_miss_limit_and_timeout);
  RUN_TEST(test_parse_loadcell_payload);
  RUN_TEST(test_parse_pressure_payload);
  RUN_TEST(test_parse_encoder_payload_reconstructs_signed_position);
  return UNITY_END();
}
