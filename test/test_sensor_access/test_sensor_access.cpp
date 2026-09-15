#include <unity.h>
#include <thread>
#include "sensors/SensorAccess.h"

void setUp() { nowMs = 0; delayedMs = 0; delayCalls = 0; }
void tearDown() {}

void test_inflight_read_excludes_ui_until_released() {
  SensorAccess access;
  TEST_ASSERT_TRUE(access.tryAcquire());
  TEST_ASSERT_FALSE(access.tryAcquire());
  TEST_ASSERT_FALSE(access.tryAcquire());
  access.release();
  TEST_ASSERT_TRUE(access.tryAcquire());
  TEST_ASSERT_FALSE(access.tryAcquire());
  access.release();
}

void test_rejected_attempt_does_not_release_current_owner() {
  SensorAccess access;
  for (unsigned cycle = 0; cycle < 100; ++cycle) {
    TEST_ASSERT_TRUE(access.tryAcquire());
    for (unsigned attempt = 0; attempt < 10; ++attempt) {
      TEST_ASSERT_FALSE(access.tryAcquire());
    }
    access.release();
  }
  TEST_ASSERT_TRUE(access.tryAcquire());
  access.release();
}

void test_timed_acquisition_expires_across_clock_wrap_without_releasing_owner() {
  SensorAccess access;
  TEST_ASSERT_TRUE(access.tryAcquire());
  nowMs = UINT32_MAX - 100;
  const uint32_t started = nowMs;
  TEST_ASSERT_FALSE(access.acquire());
  TEST_ASSERT_EQUAL_UINT32(200, static_cast<uint32_t>(nowMs - started));
  TEST_ASSERT_FALSE(access.tryAcquire());
  access.release();
  TEST_ASSERT_TRUE(access.acquire());
  TEST_ASSERT_EQUAL_UINT(200, delayedMs);
  access.release();
}

void test_zero_timeout_is_nonblocking() {
  SensorAccess access;
  TEST_ASSERT_TRUE(access.acquire(0));
  TEST_ASSERT_FALSE(access.acquire(0));
  TEST_ASSERT_EQUAL_UINT(0, delayCalls);
  access.release();
}

void test_two_threads_update_exclusively() {
  SensorAccess access;
  unsigned protectedValue = 0;
  auto update = [&]() {
    for (unsigned iteration = 0; iteration < 10000; ++iteration) {
      while (!access.tryAcquire()) std::this_thread::yield();
      ++protectedValue;
      access.release();
    }
  };
  std::thread reader(update);
  std::thread interface(update);
  reader.join();
  interface.join();
  TEST_ASSERT_EQUAL_UINT(20000, protectedValue);
  TEST_ASSERT_TRUE(access.tryAcquire());
  access.release();
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_inflight_read_excludes_ui_until_released);
  RUN_TEST(test_rejected_attempt_does_not_release_current_owner);
  RUN_TEST(test_timed_acquisition_expires_across_clock_wrap_without_releasing_owner);
  RUN_TEST(test_zero_timeout_is_nonblocking);
  RUN_TEST(test_two_threads_update_exclusively);
  return UNITY_END();
}