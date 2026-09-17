#ifndef APP_ANALYZER_POLICY_H
#define APP_ANALYZER_POLICY_H

#include <cstdint>

namespace app::policy {

constexpr uint32_t CALIBRATION_SAMPLE_MS = 250;
constexpr uint32_t CALIBRATION_STABILITY_WINDOW_MS = 2000;
constexpr uint8_t CALIBRATION_WINDOW_SAMPLES = CALIBRATION_STABILITY_WINDOW_MS / CALIBRATION_SAMPLE_MS + 1;
constexpr uint32_t CALIBRATION_MINIMUM_MS = 5000;
constexpr uint32_t CALIBRATION_TIMEOUT_MS = 10000;
constexpr float O2_CALIBRATION_STABILITY_MV = 0.25f;
constexpr float HE_CALIBRATION_STABILITY_MV = 1.0f;
constexpr float O2_CALIBRATION_DRIFT_MV_PER_SECOND = 0.05f;
constexpr float HE_CALIBRATION_DRIFT_MV_PER_SECOND = 0.2f;

constexpr float O2_AIR_CALIBRATION_MIN_MV = 5.0f;
constexpr float O2_AIR_CALIBRATION_MAX_MV = 20.0f;
constexpr float O2_AIR_CALIBRATION_DEFAULT_MV = 10.0f;
constexpr float O2_PURE_CALIBRATION_MIN_MV = 30.0f;
constexpr float O2_PURE_CALIBRATION_MAX_MV = 100.0f;
constexpr float HE_CALIBRATION_MIN_MV = 400.0f;
constexpr float HE_CALIBRATION_DEFAULT_MV = 620.0f;

constexpr uint32_t CO_WARMUP_MS = 14000;
constexpr float CO_WARMUP_PPM = 0.0f;
constexpr float HE_WARMUP_TEMPERATURE_C = 30.0f;

}

#endif
