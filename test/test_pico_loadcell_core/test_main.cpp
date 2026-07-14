#include <PicoLoadcellCore.h>
#include <unity.h>

using namespace Haptic;

void test_loadcell_payload_uses_first_two_slots_and_zeros_reserved_slots() {
  int16_t values[kMaxPayloadWords] = {};

  PicoLoadcell::buildPayload(123, -456, values);

  TEST_ASSERT_EQUAL_INT16(123, values[0]);
  TEST_ASSERT_EQUAL_INT16(-456, values[1]);
  for (uint8_t i = 2; i < kMaxPayloadWords; ++i) {
    TEST_ASSERT_EQUAL_INT16(0, values[i]);
  }
}

void test_loadcell_payload_clamps_to_packet_int16_range() {
  int16_t values[kMaxPayloadWords] = {};

  PicoLoadcell::buildPayload(50000, -50000, values);

  TEST_ASSERT_EQUAL_INT16(32767, values[0]);
  TEST_ASSERT_EQUAL_INT16(-32768, values[1]);
}

void test_loadcell_status_faults_when_either_hx711_times_out() {
  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_OK,
                          PicoLoadcell::statusForTimeoutFlags(false, false));
  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_SENSOR_FAULT,
                          PicoLoadcell::statusForTimeoutFlags(true, false));
  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_SENSOR_FAULT,
                          PicoLoadcell::statusForTimeoutFlags(false, true));
  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_SENSOR_FAULT,
                          PicoLoadcell::statusForTimeoutFlags(true, true));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_loadcell_payload_uses_first_two_slots_and_zeros_reserved_slots);
  RUN_TEST(test_loadcell_payload_clamps_to_packet_int16_range);
  RUN_TEST(test_loadcell_status_faults_when_either_hx711_times_out);
  return UNITY_END();
}
