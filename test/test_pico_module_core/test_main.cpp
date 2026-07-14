#include <PicoModuleCore.h>
#include <unity.h>

using namespace Haptic;

void test_initialize_packet_sets_booting_metadata_and_valid_checksum() {
  ModulePacket packet{};

  PicoCore::initializePacket(packet, MODULE_KIND_PRESSURE, 321);

  TEST_ASSERT_EQUAL_UINT8(kProtocolVersion, packet.protocolVersion);
  TEST_ASSERT_EQUAL_UINT8(MODULE_KIND_PRESSURE, packet.moduleKind);
  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_BOOTING, packet.status);
  TEST_ASSERT_EQUAL_UINT8(0, packet.sequence);
  TEST_ASSERT_EQUAL_UINT16(321, packet.idAdc);
  for (uint8_t i = 0; i < kMaxPayloadWords; ++i) {
    TEST_ASSERT_EQUAL_INT16(0, packet.payload[i]);
  }
  TEST_ASSERT_TRUE(validatePacket(packet));
}

void test_publish_packet_increments_sequence_updates_id_and_zero_fills_payload() {
  ModulePacket packet{};
  PicoCore::initializePacket(packet, MODULE_KIND_LOADCELL, 100);
  const int16_t values[] = {11, -22};

  PicoCore::publishPacket(packet, MODULE_STATUS_OK, 200, values, 2);

  TEST_ASSERT_EQUAL_UINT8(kProtocolVersion, packet.protocolVersion);
  TEST_ASSERT_EQUAL_UINT8(MODULE_KIND_LOADCELL, packet.moduleKind);
  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_OK, packet.status);
  TEST_ASSERT_EQUAL_UINT8(1, packet.sequence);
  TEST_ASSERT_EQUAL_UINT16(200, packet.idAdc);
  TEST_ASSERT_EQUAL_INT16(11, packet.payload[0]);
  TEST_ASSERT_EQUAL_INT16(-22, packet.payload[1]);
  for (uint8_t i = 2; i < kMaxPayloadWords; ++i) {
    TEST_ASSERT_EQUAL_INT16(0, packet.payload[i]);
  }
  TEST_ASSERT_TRUE(validatePacket(packet));
}

void test_publish_packet_sequence_wraps_naturally() {
  ModulePacket packet{};
  PicoCore::initializePacket(packet, MODULE_KIND_ENCODER, 0);
  packet.sequence = 255;
  finalizePacket(packet);
  const int16_t values[kMaxPayloadWords] = {};

  PicoCore::publishPacket(packet, MODULE_STATUS_OK, 0, values, kMaxPayloadWords);

  TEST_ASSERT_EQUAL_UINT8(0, packet.sequence);
  TEST_ASSERT_TRUE(validatePacket(packet));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_initialize_packet_sets_booting_metadata_and_valid_checksum);
  RUN_TEST(test_publish_packet_increments_sequence_updates_id_and_zero_fills_payload);
  RUN_TEST(test_publish_packet_sequence_wraps_naturally);
  return UNITY_END();
}
