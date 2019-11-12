import shv.log;
import std.algorithm;
import std.range;
import std.stdio;

// define topic log
alias logBar = logMessage!"bar";

int main(string[] args)
{
	args = globalLog.setCLIOptions(args);

	if(!find(args, "--help").empty() || !find(args, "-h").empty()) {
		writeln(Log.cliHelp());
		return 0;
	}

	logInfo("For help run with -h or --help");
	logInfo("tresholds:", globalLog.tresholdsInfo());
	logInfo("command line args not used by log:", args);

	logDebug("Debug message");
	logMessage("Msg message");
	logInfo("Info message");
	logWarning("Warning message");
	logError("Error message");

	// log with topic, will be ignored without CLI arg '-v foo'
	logMessage!"foo"("Message with topic 'foo'");

	// not logged params are not evaluated thanks to lazy func params
	int x = 0;
	logBar("Message with topic 'bar' #1", ++x);
	assert(x == 0);
	globalLog.setTopicTresholds("bar:M");
	logBar("Message with topic 'bar' #2", ++x);
	assert(x == 1);

	// colored log
	logInfo!("", Log.Color.Brown)("Info message, brown color no topic");
	logInfo!("bar", Log.Color.Green)("Info message, green color topic 'bar'");

	return 0;
}
