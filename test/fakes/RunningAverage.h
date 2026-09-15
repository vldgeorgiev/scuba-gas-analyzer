#ifndef TEST_RUNNING_AVERAGE_H
#define TEST_RUNNING_AVERAGE_H

class RunningAverage {
public:
  explicit RunningAverage(unsigned capacity) : _capacity(capacity) {}
  bool addValue(float value) {
    if (_count == _capacity) return false;
    _sum += value;
    ++_count;
    return true;
  }
  float getAverage() const { return _count ? _sum / _count : 0.0f; }

private:
  unsigned _capacity;
  unsigned _count = 0;
  float _sum = 0.0f;
};

#endif