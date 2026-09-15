#ifndef SENSOR_CONVERSIONS_H
#define SENSOR_CONVERSIONS_H

namespace conversions {

float o2Percentage(float millivolts, float calibration21, float calibration100);
float heCorrectedMillivolts(float millivolts, float o2Percentage);
float hePercentage(float millivolts, float calibration100);
float coPpm(float millivolts);
float temperatureCelsius(float millivolts);
float maximumOperatingDepth(float po2, float o2Percentage);
bool toInt(float value, int& result);

}

#endif