module shv.log;

import std.array;
import std.conv;
import std.stdio;
import std.ascii;
import std.string;
import std.algorithm;

struct Log
{
public:
	enum Level : ubyte {Invalid = 0, Fatal, Error, Warning, Info, Message, Debug};
	enum Color : byte {Default = -1, Black=0, Red, Green, Brown, Blue, Magenta, Cyan, LightGray, Gray, LightRed, LightGreen, Yellow, LightBlue, LightMagenta, LightCyan, White};

	static string levelToString(Level level)
	{
		return std.conv.to!string( level );
	}

	static string cliHelp()
	{
		static const string ret = r"
-d [<pattern>]:[D|M|I|W|E]
	Set module name log treshold
-v, --topic [<pattern>]:[D|M|I|W|E]
	Set topic log treshold
		set treshold for all files or topics containing pattern to treshold D|I|W|E
		when pattern is not set, set treshold for any filename or topic
		when treshold is not set, set treshold D (Debug) for all files or topics containing pattern
		when nothing is not set, set treshold D (Debug) for all files or topics
		Examples:
			-d		set treshold D (Debug) for all files or topics
			-d :W		set treshold W (Warning) for all files or topics
			-d foo,bar	set treshold D for all files or topics containing 'foo' or 'bar'
			-d bar:W	set treshold W (Warning) for all files or topics containing 'bar'";
		return ret;
	}

	string[] setCLIOptions(string[] args)
	{
		string[] ret;
		for(int i = 1; i < args.length; i++) {
			string s = args[i];
			if(s == "-d" || s == "-v" || s == "--verbose") {
				bool use_topics = (s != "-d");
				i++;
				string tresholds = (i < args.length)? args[i]: "";
				if(tresholds.empty() || (!tresholds.empty() && tresholds[0] == '-')) {
					i--;
					tresholds = ":D";
				}
				parseTresholdsString(tresholds, use_topics? options.topicTresholds: options.moduleTresholds);
			}
			else {
				ret ~= s;
			}
		}
		ret = args[0] ~ ret;
		return ret;
	}

	string tresholdsInfo()
	{
		string ret = "-d ";
		if(options.moduleTresholds.empty())
			ret ~= ":I";
		else
			ret ~= tresholdsToString(options.moduleTresholds);
		if(!options.topicTresholds.empty()) {
			ret ~= " -v ";
			ret ~= tresholdsToString(options.topicTresholds);
		}
		return ret;
	}

	void setTopicTresholds(string tresholds)
	{
		options.topicTresholds.clear();
		parseTresholdsString(tresholds, options.topicTresholds);
	}

	struct LogContext
	{
	public:
		string moduleName;
		int line;
		string topic;
		Level level;
		Color color;

		bool isColorSet() const {return color != Color.Default;}
		bool isModuleSet() const {return moduleName.length > 0;}
		bool isTopicSet() const {return topic.length > 0;}
	};

	void log(T...)(LogContext context, lazy T args)
	{
		if(context.level == Level.Fatal)
			assert(0, makeString(args));
		if(!shouldLog(context))
			return;
		string msg = makeString(args);
		defaultMessageHandler(context, msg);
	}

private:
	void defaultMessageHandler(const ref LogContext ctx, string message)
	{
			writeWithDefaultFormat(stderr, is_tty(), ctx, message);
	}

	static void parseTresholdsString(string tresholds, ref Level[string] treshold_map)
	{
		// split on ','
		foreach (treshold; tresholds.split(',')) {
			auto topic_level = treshold.split(':');
			auto topic = topic_level.length > 0? topic_level[0]: "";
			auto levels = topic_level.length > 1? topic_level[1]: "";
			char levelc = levels.length > 0? std.ascii.toUpper(levels[0]): 'D';
			auto level = Log.Level.Debug;
			switch(levelc) {
			case 'D': level = Log.Level.Debug; break;
			case 'M': level = Log.Level.Message; break;
			case 'I': level = Log.Level.Info; break;
			case 'W': level = Log.Level.Warning; break;
			case 'E': level = Log.Level.Error; break;
			default: level = Log.Level.Info; break;
			}
			treshold_map[topic] = level;
						}
	}

	static bool is_tty()
	{
		version(Posix) {
			static bool res;
			static bool isatty_called = false;
			if(!isatty_called) {
				isatty_called = true;
				import core.sys.posix.unistd;
				res = isatty(0) > 0;
			}
			return res;
		}
		return false;
	}
	static string tresholdsToString(Log.Level[string] tresholds)
	{
		string ret;
		foreach (k, v; tresholds) {
			if(!ret.empty)
				ret ~= ',';
			ret ~= k ~ ':';
			ret ~= Log.levelToString(v)[0];
		}
		return ret;
	}

	enum TTYColor {Black=0, Red, Green, Brown, Blue, Magenta, Cyan, LightGray, Gray, LightRed, LightGreen, Yellow, LightBlue, LightMagenta, LightCyan, White};
	/*
	ESC[T;FG;BGm
	T:
	  0 - normal
	  1 - bright
	  2 - dark
	  3 - italic
	  4 - underline
	  5 - blink
	  6 - ???
	  7 - inverse
	  8 - foreground same as background
	FG:
	  30 - black
	  31 - red
	  32 - green
	  33 - yellow
	  34 - blue
	  35 - magenta
	  36 - cyan
	  37 - white
	BG:
	  40 - black
	  41 - red
	  42 - green
	  43 - yellow
	  44 - blue
	  45 - magenta
	  46 - cyan
	  47 - white
	*/
	void set_TTY_color(bool is_tty, File os, TTYColor color, bool bg_color = false)
	{
		if(is_tty)
			os.write("\033[", (color / 8)? '1': '0', ';', bg_color? '4': '3', cast(char)('0' + (color % 8)), 'm');
	}

	void writeWithDefaultFormat(File os, bool is_colorized, const ref LogContext context, const string msg)
	{
		import std.datetime.systime : SysTime, Clock;
		SysTime currentTime = Clock.currTime();

		set_TTY_color(is_colorized, os, TTYColor.Green);
		os.write(currentTime.toISOExtString());
		set_TTY_color(is_colorized, os, TTYColor.Brown);
		os.write('[', context.moduleName, ':', context.line, ']');
		if(context.topic.length) {
			set_TTY_color(is_colorized, os, TTYColor.White);
			os.write('(', context.topic, ')');
		}
		switch(context.level) {
		case Log.Level.Fatal:
			set_TTY_color(is_colorized, os, context.isColorSet()? cast(TTYColor) context.color: cast(TTYColor) Log.Color.LightRed);
			os.write("|F|");
			break;
		case Log.Level.Error:
			set_TTY_color(is_colorized, os, context.isColorSet()? cast(TTYColor) context.color: cast(TTYColor) Log.Color.LightRed);
			os.write("|E|");
			break;
		case Log.Level.Warning:
			set_TTY_color(is_colorized, os, context.isColorSet()? cast(TTYColor) context.color: cast(TTYColor) Log.Color.LightMagenta);
			os.write("|W|");
			break;
		case Log.Level.Info:
			set_TTY_color(is_colorized, os, context.isColorSet()? cast(TTYColor) context.color: cast(TTYColor) Log.Color.LightCyan);
			os.write("|I|");
			break;
		case Log.Level.Message:
			set_TTY_color(is_colorized, os, context.isColorSet()? cast(TTYColor) context.color: cast(TTYColor) Log.Color.Yellow);
			os.write("|M|");
			break;
		case Log.Level.Debug:
			set_TTY_color(is_colorized, os, context.isColorSet()? cast(TTYColor) context.color: cast(TTYColor) Log.Color.LightGray);
			os.write("|D|");
			break;
		default:
			set_TTY_color(is_colorized, os, context.isColorSet()? cast(TTYColor) context.color: cast(TTYColor) Log.Color.LightRed);
			os.write("|?|");
			break;
		}
		os.write(" ", msg);

		if(is_colorized)
			os.write("\33[0m");

		os.writeln("");
		os.flush();
	}

	private bool shouldLog(const ref LogContext context)
	{
		debug {
		}
		else {
			if(context.level == Log.Level.Debug)
				return false;
		}
		const bool topic_set = (context.isTopicSet());
		if(!topic_set && options.moduleTresholds.empty())
			return context.level <= Log.Level.Info; // when tresholds are not set, log non-topic INFO messages

		string haystack = "";
		if(topic_set)
			haystack = context.topic;
		else
			haystack = context.moduleName;

		auto tresholds = topic_set? options.topicTresholds: options.moduleTresholds;
		foreach(needle, level; tresholds) {
			auto find_res = haystack.find(needle);
			if(!find_res.empty) {
				return (context.level <= level);
			}
		}
		// not found in tresholds
		//if(!topic_set)
		//	return level <= Level.Info; // log non-topic INFO messages
		return false;
	}

	static string toLogString(T)(T t)
	{
		return to!string(t);
	}

	static string makeString(T...)(T args)
	{
		string msg = "";
		foreach(i, t; args) {
			if(msg.length)
				msg ~= " ";
			msg ~= toLogString(t);
		}
		return msg;
	}
private:
	struct Options
	{
		Log.Level[string] moduleTresholds;
		Log.Level[string] topicTresholds;
		string[string] registeredTopics;
		//MessageHandler messageHandler = defaultMessageHandler;
	};

	Options options;

};

Log globalLog;

template LogSpec(Log.Level log_level)
{
	template logImpl(string topic = "", Log.Color color = Log.Color.Default) {
		void logImpl(
			int line = __LINE__,
			//string file = __FILE__,
			//string funcName = __FUNCTION__,
			//string prettyFuncName = __PRETTY_FUNCTION__,
			string moduleName = __MODULE__,
			A...) (lazy A args)
		{
			auto ctx = Log.LogContext();
			ctx.moduleName = moduleName;
			ctx.line = line;
			ctx.topic = topic;
			ctx.level = log_level;
			ctx.color = color;
			globalLog.log(ctx, args);
		}
	}
}

alias logDebug = LogSpec!(Log.Level.Debug).logImpl;
alias logMessage = LogSpec!(Log.Level.Message).logImpl;
alias logMsg = LogSpec!(Log.Level.Message).logImpl;
alias logInfo = LogSpec!(Log.Level.Info).logImpl;
alias logWarning = LogSpec!(Log.Level.Warning).logImpl;
alias logError = LogSpec!(Log.Level.Error).logImpl;
alias logFatal = LogSpec!(Log.Level.Fatal).logImpl;

