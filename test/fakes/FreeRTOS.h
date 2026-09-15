#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H

#include <cstdint>
#include <cstring>
#include <vector>

struct FakeQueue {
  explicit FakeQueue(size_t itemSize) : data(itemSize) {}
  std::vector<uint8_t> data;
  unsigned sends = 0;
};

using QueueHandle_t = FakeQueue*;
constexpr uint32_t portMAX_DELAY = UINT32_MAX;

inline int xQueueSend(QueueHandle_t queue, const void* item, uint32_t) {
  std::memcpy(queue->data.data(), item, queue->data.size());
  ++queue->sends;
  return 1;
}

#endif