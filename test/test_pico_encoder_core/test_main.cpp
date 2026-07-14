#include <PicoEncoderCore.h>
#include <unity.h>

using namespace Haptic;

void test_encoder_quadrature_table_accepts_forward_and_reverse_steps() {
  TEST_ASSERT_EQUAL_INT8(1, PicoEncoder::quadratureDelta(0b00, 0b01));
  TEST_ASSERT_EQUAL_INT8(1, PicoEncoder::quadratureDelta(0b01, 0b11));
  TEST_ASSERT_EQUAL_INT8(1, PicoEncoder::quadratureDelta(0b11, 0b10));
  TEST_ASSERT_EQUAL_INT8(1, PicoEncoder::quadratureDelta(0b10, 0b00));

  TEST_ASSERT_EQUAL_INT8(-1, PicoEncoder::quadratureDelta(0b00, 0b10));
  TEST_ASSERT_EQUAL_INT8(-1, PicoEncoder::quadratureDelta(0b10, 0b11));
  TEST_ASSERT_EQUAL_INT8(-1, PicoEncoder::quadratureDelta(0b11, 0b01));
  TEST_ASSERT_EQUAL_INT8(-1, PicoEncoder::quadratureDelta(0b01, 0b00));
}

void test_encoder_quadrature_table_rejects_no_change_and_invalid_jumps() {
  TEST_ASSERT_EQUAL_INT8(0, PicoEncoder::quadratureDelta(0b00, 0b00));
  TEST_ASSERT_EQUAL_INT8(0, PicoEncoder::quadratureDelta(0b00, 0b11));
  TEST_ASSERT_EQUAL_INT8(0, PicoEncoder::quadratureDelta(0b01, 0b10));
}

void test_encoder_velocity_uses_tenth_rpm_units() {
  // 20 counts / 50 ms = 400 counts/sec.
  // 400 * 600 / 2400 = 100 tenths of RPM = 10.0 RPM.
  TEST_ASSERT_EQUAL_INT16(100, PicoEncoder::velocityTenthRpmFromDelta(20, 50));
  TEST_ASSERT_EQUAL_INT16(-100, PicoEncoder::velocityTenthRpmFromDelta(-20, 50));
}

void test_encoder_velocity_update_window_is_50_ms() {
  TEST_ASSERT_FALSE(PicoEncoder::shouldUpdateVelocity(49));
  TEST_ASSERT_TRUE(PicoEncoder::shouldUpdateVelocity(50));
}

void test_encoder_direction_uses_sign_of_position_delta() {
  TEST_ASSERT_EQUAL_INT16(1, PicoEncoder::directionFromDelta(20));
  TEST_ASSERT_EQUAL_INT16(0, PicoEncoder::directionFromDelta(0));
  TEST_ASSERT_EQUAL_INT16(-1, PicoEncoder::directionFromDelta(-20));
}

void test_encoder_payload_splits_and_reconstructs_signed_position() {
  int16_t values[kMaxPayloadWords] = {};
  const int32_t positions[] = {0, 1, -1, 0x12345678, -1234567};

  for (int32_t position : positions) {
    PicoEncoder::buildPayload(position, -321, -1, values);
    TEST_ASSERT_EQUAL_INT32(position, PicoEncoder::positionFromPayload(values));
    TEST_ASSERT_EQUAL_INT16(-321, values[2]);
    TEST_ASSERT_EQUAL_INT16(-1, values[3]);
    for (uint8_t i = 4; i < kMaxPayloadWords; ++i) {
      TEST_ASSERT_EQUAL_INT16(0, values[i]);
    }
  }
}

void test_encoder_velocity_clamps_to_packet_int16_range() {
  TEST_ASSERT_EQUAL_INT16(32767,
                          PicoEncoder::velocityTenthRpmFromDelta(10000000, 1));
  TEST_ASSERT_EQUAL_INT16(-32768,
                          PicoEncoder::velocityTenthRpmFromDelta(-10000000, 1));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_encoder_quadrature_table_accepts_forward_and_reverse_steps);
  RUN_TEST(test_encoder_quadrature_table_rejects_no_change_and_invalid_jumps);
  RUN_TEST(test_encoder_velocity_uses_tenth_rpm_units);
  RUN_TEST(test_encoder_velocity_update_window_is_50_ms);
  RUN_TEST(test_encoder_direction_uses_sign_of_position_delta);
  RUN_TEST(test_encoder_payload_splits_and_reconstructs_signed_position);
  RUN_TEST(test_encoder_velocity_clamps_to_packet_int16_range);
  return UNITY_END();
}
