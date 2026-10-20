#ifndef TEST_UI_LOG_H
#define TEST_UI_LOG_H

#include <cstdio>
#include <cstring>

#define log_e(...) ((void)0)
#define log_w(...) ((void)0)
#define log_i(...) ((void)0)
#define log_d(...) ((void)0)

enum class UiLogLevel { None, Warning, Error, Unchanged };
class UiLog {
public:
	static constexpr size_t kLogSnapshotSize = 500;

	static UiLog& getInstance() { static UiLog instance; return instance; }
	void addLogEntry(const char* text, UiLogLevel newLevel) {
		if (newLevel != UiLogLevel::Unchanged) level = newLevel;
		std::snprintf(snapshot, sizeof(snapshot), "[0] %s\n", text);
	}
	void clearLog() { level = UiLogLevel::None; snapshot[0] = '\0'; }
	UiLogLevel getLevel() const { return level; }
	void copyLogTo(char (&out)[kLogSnapshotSize]) const { std::memcpy(out, snapshot, sizeof(snapshot)); }

private:
	UiLogLevel level = UiLogLevel::None;
	char snapshot[kLogSnapshotSize] = {};
};
inline void logUi(const char* text, UiLogLevel level) { UiLog::getInstance().addLogEntry(text, level); }

#endif