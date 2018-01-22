#pragma once

#include "../shvcoreglobal.h"

#include <sstream>
#include <functional>
#include <vector>

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT ShvLog
{
public:
	enum class Level : int { Invalid = -1, Fatal, Error, Warning, Info, Debug };

	class SHVCORE_DECL_EXPORT LogContext
	{
	public:
		LogContext() : file(nullptr), function(nullptr), category(nullptr), line(0) {}
		LogContext(const char *file_name, int line_number, const char *function_name, const char *category_name = nullptr)
			: file(file_name), function(function_name), category(category_name), line(line_number) {}

		const char *file;
		const char *function;
		const char *category;
		int line;
	};
	using MessageOutput = std::function<void (Level level, const LogContext &context, const std::string &message)>;
private:
	struct Stream
	{
		Stream(Level level, const LogContext &context, std::streambuf *buff = nullptr)
			: ts((buff == nullptr)? &buffer: buff)
			, level(level)
			, context(context)
		{ }

		std::ostream ts;
		std::stringbuf buffer;
		int ref = 1;
		bool space = true;
		//bool message_output;
		Level level = Level::Debug;
		LogContext context;
	} *stream;
public:
	inline ShvLog(Level level, const LogContext &context, std::streambuf *buff = nullptr) : stream(new Stream(level, context, buff)) {}
	inline ShvLog(const ShvLog &o) : stream(o.stream) { ++stream->ref; }
	inline ShvLog &operator=(const ShvLog &other);
	~ShvLog();

	static std::vector<std::string> setGlobalTresholds(int argc, char *argv[]);
	static bool isMatchingLogFilter(ShvLog::Level level, const ShvLog::LogContext &log_context);
	static void setMessageOutput(MessageOutput out);
	static ShvLog::MessageOutput messageOutput();

	inline void swap(ShvLog &other) noexcept { std::swap(stream, other.stream); }
	//ShvLog &resetFormat();
	inline ShvLog &space() { stream->space = true; stream->ts << ' '; return *this; }
	inline ShvLog &nospace() { stream->space = false; return *this; }
	inline ShvLog &maybeSpace() { if (stream->space) stream->ts << ' '; return *this; }

	template<typename T>
	inline ShvLog &operator<<(T t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(const void * t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(std::nullptr_t) { stream->ts << "(nullptr)"; return maybeSpace(); }

	static std::string modulesLogInfo();
	static std::string categoriesLogInfo();
	static const char *levelToString(ShvLog::Level level);
	static const char *logCLIHelp();
};

inline ShvLog &ShvLog::operator=(const ShvLog &other)
{
	if (this != &other) {
		ShvLog copy(other);
		std::swap(stream, copy.stream);
	}
	return *this;
}

}}
