#ifndef TEST_UI_LOG_H
#define TEST_UI_LOG_H

#define log_e(...) ((void)0)
#define log_w(...) ((void)0)
#define log_i(...) ((void)0)
#define log_d(...) ((void)0)

enum class UiLogLevel { Error };
inline void logUi(const char*, UiLogLevel) {}

#endif