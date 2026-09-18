#ifndef TEST_UI_LOG_H
#define TEST_UI_LOG_H

#define log_e(...) ((void)0)
#define log_w(...) ((void)0)
#define log_i(...) ((void)0)
#define log_d(...) ((void)0)

enum class UiLogLevel { None, Warning, Error, Unchanged };
class UiLog {
public:
	static UiLog& getInstance() { static UiLog instance; return instance; }
	void addLogEntry(const char*, UiLogLevel newLevel) {
		if (newLevel != UiLogLevel::Unchanged) level = newLevel;
	}
	void clearLog() { level = UiLogLevel::None; }
	UiLogLevel getLevel() const { return level; }
	const char* getLogAsCString() const { return ""; }

private:
	UiLogLevel level = UiLogLevel::None;
};
inline void logUi(const char* text, UiLogLevel level) { UiLog::getInstance().addLogEntry(text, level); }

#endif