#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H

#include <cstdint>
#include <cstring>
#include <vector>

struct FakeQueue {
  explicit FakeQueue(size_t itemSize) : data(itemSize) {}
  std::vector<uint8_t> data;
  unsigned sends = 0;
  bool occupied = false;
};

using QueueHandle_t = FakeQueue*;
constexpr uint32_t portMAX_DELAY = UINT32_MAX;

inline int xQueueOverwrite(QueueHandle_t queue, const void* item) {
  std::memcpy(queue->data.data(), item, queue->data.size());
  ++queue->sends;
  queue->occupied = true;
  return 1;
}

inline int xQueueReceive(QueueHandle_t queue, void* item, uint32_t) {
  if (!queue->occupied) return 0;
  std::memcpy(item, queue->data.data(), queue->data.size());
  queue->occupied = false;
  return 1;
}

#endif