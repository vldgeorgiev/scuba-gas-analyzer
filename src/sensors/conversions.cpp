#include "conversions.h"

#include <cmath>
#include <limits>

namespace conversions {

float o2Percentage(float millivolts, float calibration21, float calibration100) {
  if (!std::isfinite(millivolts) || millivolts < 0 || millivolts > 100 ||
      !std::isfinite(calibration21) || calibration21 < 5) return NAN;
  if (!std::isnan(calibration100) &&
      (!std::isfinite(calibration100) || calibration100 <= calibration21)) return NAN;
  float percentage;
  if (!std::isnan(calibration100)) {
    percentage = 20.9 + 79.1 * (millivolts - calibration21) / (calibration100 - calibration21);
  } else {
    percentage = 20.9 / calibration21 * millivolts;
  }
  if (!std::isfinite(percentage)) return NAN;
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  return percentage;
}

float heCorrectedMillivolts(float millivolts, float o2Percentage) {
  if (!std::isfinite(millivolts) || millivolts < 0 ||
      !std::isfinite(o2Percentage) || o2Percentage < 0 || o2Percentage > 100) return NAN;
  if (o2Percentage > 40) {
    if (o2Percentage > 89) { millivolts = millivolts - 17; }
    else if (o2Percentage > 82) { millivolts -= 16; }
    else if (o2Percentage > 75) { millivolts -= 15; }
    else if (o2Percentage > 71) { millivolts -= 14; }
    else if (o2Percentage > 66) { millivolts -= 13; }
    else if (o2Percentage > 62) { millivolts -= 12; }
    else if (o2Percentage > 57) { millivolts -= 11; }
    else if (o2Percentage > 52) { millivolts -= 10; }
    else if (o2Percentage > 48) { millivolts -= 9; }
  }
  return millivolts >= 0 ? millivolts : NAN;
}

float hePercentage(float millivolts, float calibration100) {
  if (!std::isfinite(millivolts) || millivolts < 0 ||
      !std::isfinite(calibration100) || calibration100 <= 0) return NAN;
  const double FIXED_MAX_MV_100 = 621.2;
  double adjustmentFactor = FIXED_MAX_MV_100 / calibration100;
  double adjustedMv = millivolts * adjustmentFactor;
  const double percentage = + 1.098e-7 * std::pow(adjustedMv, 3)
         - 4.584e-5 * std::pow(adjustedMv, 2)
         + (0.1471 * adjustedMv);
  if (!std::isfinite(percentage) || percentage < 0 ||
      percentage > std::numeric_limits<float>::max()) return NAN;
  return static_cast<float>(percentage);
}

float coPpm(float millivolts) {
  if (!std::isfinite(millivolts) || millivolts < 0) return NAN;
  const float V_MIN = 400;
  const float V_MAX = 2000;
  const float PPM_MAX = 500.0;
  return (millivolts - V_MIN) * (PPM_MAX / (V_MAX - V_MIN));
}

float temperatureCelsius(float millivolts) {
  if (!std::isfinite(millivolts)) return NAN;
  const float scaled = millivolts * 10;
  if (!std::isfinite(scaled)) return NAN;
  return std::round(scaled) / 10 / 10;
}

float maximumOperatingDepth(float po2, float o2Percentage) {
  if (!std::isfinite(po2) || po2 <= 0 || !std::isfinite(o2Percentage) ||
      o2Percentage <= 0 || o2Percentage > 100) return NAN;
  const float depth = po2 / o2Percentage * 1000 - 10;
  return std::isfinite(depth) ? depth : NAN;
}

bool toInt(float value, int& result) {
  if (!std::isfinite(value) ||
      static_cast<double>(value) < std::numeric_limits<int>::min() ||
      static_cast<double>(value) > std::numeric_limits<int>::max()) return false;
  result = static_cast<int>(value);
  return true;
}

}