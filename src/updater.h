#include <WiFi.h>
#include <vector>
// #include <string>

// #include "utils.h"

std::vector<String> scanWifiNetworks() {
  std::vector<String> ssidList;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // ensure a clean scan
  // delay(100);        // short delay before scanning

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    ssidList.push_back(WiFi.SSID(i));
  }

  return ssidList;
}
