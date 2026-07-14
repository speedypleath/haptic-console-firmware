#include <PicoPressureCore.h>
#include <unity.h>

using namespace Haptic;

void test_pressure_payload_maps_raw_adc_to_centikpa_and_diagnostics() {
  int16_t values[kMaxPayloadWords] = {};

  const ModuleStatus status = PicoPressure::buildPayload(
      PicoPressure::kZeroAdcCount + PicoPressure::kAdcPerKpa * 5, values);

  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_OK, status);
  TEST_ASSERT_EQUAL_INT16(500, values[0]);
  TEST_ASSERT_EQUAL_INT16(PicoPressure::kZeroAdcCount + PicoPressure::kAdcPerKpa * 5,
                          values[1]);
  TEST_ASSERT_EQUAL_INT16(PicoPressure::kZeroAdcCount, values[2]);
  for (uint8_t i = 3; i < kMaxPayloadWords; ++i) {
    TEST_ASSERT_EQUAL_INT16(0, values[i]);
  }
}

void test_pressure_clamps_negative_pressure_to_zero_without_immediate_fault() {
  int16_t values[kMaxPayloadWords] = {};

  const ModuleStatus status = PicoPressure::buildPayload(
      PicoPressure::kZeroAdcCount - PicoPressure::kOverrangeAdc, values);

  TEST_ASSERT_EQUAL_UINT8(MODULE_STATUS_OK, status);
  TEST_ASSERT_EQUAL_INT16(0, values[0]);
}

void test_pressure_faults_below_low_limit_and_above_high_limit() {
  TEST_ASSERT_EQUAL_UINT8(
      MODULE_STATUS_SENSOR_FAULT,
      PicoPressure::statusForRaw(PicoPressure::kZeroAdcCount -
                                 PicoPressure::kOverrangeAdc - 1));

  TEST_ASSERT_EQUAL_UINT8(
      MODULE_STATUS_SENSOR_FAULT,
      PicoPressure::statusForRaw(PicoPressure::kZeroAdcCount +
                                 PicoPressure::kAdcPerKpa * 10 +
                                 PicoPressure::kOverrangeAdc + 1));
}

void test_pressure_handles_invalid_calibration_safely() {
  TEST_ASSERT_EQUAL_INT16(0, PicoPressure::pressureCentikpaFromRaw(1000, 100, 0));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_pressure_payload_maps_raw_adc_to_centikpa_and_diagnostics);
  RUN_TEST(test_pressure_clamps_negative_pressure_to_zero_without_immediate_fault);
  RUN_TEST(test_pressure_faults_below_low_limit_and_above_high_limit);
  RUN_TEST(test_pressure_handles_invalid_calibration_safely);
  return UNITY_END();
}
