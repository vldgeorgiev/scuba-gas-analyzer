#ifndef TEST_PREFERENCES_H
#define TEST_PREFERENCES_H

#include <cstdint>
#include <map>
#include <string>

class Preferences {
public:
  inline static std::map<std::string, float> values;
  inline static bool available = true;
  inline static bool failWrite = false;
  inline static unsigned writes = 0;
  inline static unsigned reads = 0;
  inline static int failAfter = -1;
  bool begin(const char*, bool) { return available; }
  void end() {}
  float getFloat(const char* key, float fallback) {
    ++reads;
    const auto found = values.find(key);
    return found == values.end() ? fallback : found->second;
  }
  bool getBool(const char* key, bool fallback) { return getFloat(key, fallback) != 0; }
  uint8_t getUChar(const char* key, uint8_t fallback) { return static_cast<uint8_t>(getFloat(key, fallback)); }
  size_t putFloat(const char* key, float value) {
    if (!available || failWrite || failAfter == 0) return 0;
    if (failAfter > 0) --failAfter;
    ++writes;
    values[key] = value;
    return sizeof(float);
  }
  size_t putBool(const char* key, bool value) { return putFloat(key, value) ? 1 : 0; }
  size_t putUChar(const char* key, uint8_t value) { return putFloat(key, value) ? 1 : 0; }
};

#endif