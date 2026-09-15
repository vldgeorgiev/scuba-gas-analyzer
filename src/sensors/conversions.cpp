#include "conversions.h"

#include <cmath>

namespace conversions {

float o2Percentage(float millivolts, float calibration21, float calibration100) {
  float percentage;
  if (!std::isnan(calibration100)) {
    percentage = 20.9 + 79.1 * (millivolts - calibration21) / (calibration100 - calibration21);
  } else {
    percentage = 20.9 / calibration21 * millivolts;
  }
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  return percentage;
}

float heCorrectedMillivolts(float millivolts, float o2Percentage) {
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
  return millivolts;
}

float hePercentage(float millivolts, float calibration100) {
  const double FIXED_MAX_MV_100 = 621.2;
  double adjustmentFactor = FIXED_MAX_MV_100 / calibration100;
  double adjustedMv = millivolts * adjustmentFactor;
  return + 1.098e-7 * std::pow(adjustedMv, 3)
         - 4.584e-5 * std::pow(adjustedMv, 2)
         + (0.1471 * adjustedMv);
}

float coPpm(float millivolts) {
  const float V_MIN = 400;
  const float V_MAX = 2000;
  const float PPM_MAX = 500.0;
  return (millivolts - V_MIN) * (PPM_MAX / (V_MAX - V_MIN));
}

float temperatureCelsius(float millivolts) {
  return std::round(millivolts * 10) / 10 / 10;
}

}