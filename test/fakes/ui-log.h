#ifndef TEST_UI_LOG_H
#define TEST_UI_LOG_H

#define log_e(...) ((void)0)
#define log_w(...) ((void)0)
#define log_i(...) ((void)0)
#define log_d(...) ((void)0)

enum class UiLogLevel { None, Warning, Error, Unchanged };
class UiLog {
public:
	static constexpr size_t kLogSnapshotSize = 500;

	static UiLog& getInstance() { static UiLog instance; return instance; }
	void addLogEntry(const char*, UiLogLevel newLevel) {
		if (newLevel != UiLogLevel::Unchanged) level = newLevel;
	}
	void clearLog() { level = UiLogLevel::None; }
	UiLogLevel getLevel() const { return level; }
	void copyLogTo(char (&out)[kLogSnapshotSize]) const { out[0] = '\0'; }

private:
	UiLogLevel level = UiLogLevel::None;
};
inline void logUi(const char* text, UiLogLevel level) { UiLog::getInstance().addLogEntry(text, level); }

#endif