#include "../../lib/log/log.h"
#include <string>
#include <iostream>
#include <fstream>

#include "NSIS.h"

using namespace std;


void help()
{
	fprintf(stderr,
			"owon_firmware_extractor [firmware_file] [general options] [logger options]\n"
			"\n"
			"  Extracts firmware from a given .exe software installer.\n"
			"  [firmware file]:\n"
			"	 Write the path to the .exe installer to extract the firmware from.\n"
			"	 Official OWON firmware may be downloaded from:\n"
			"       https://owon.com.hk/download.asp?category=digital%20oscilloscope&series=vds%20seriesSortTag=Software\n"
			"	 An unofficial firmware extraction can be found at:\n"
			"	    https://github.com/florentbr/OWON-VDS1022\n"
			"	 To use it, simply download ALL the files in fwr/ instead of using this tool\n"
			"\n"
			"  [general options]:\n"
			"    --help                        : this message...\n"
			"\n"
			"  [logger options]:\n"
			"    levels: ERROR, WARNING, NOTICE, VERBOSE, DEBUG\n"
			"    --quiet|-q                    : reduce logging level by one step\n"
			"    --verbose                     : set logging level to VERBOSE\n"
			"    --debug                       : set logging level to DEBUG\n"
			"    --trace <classname>|          : name of class with tracing messages. (Only relevant when logging level is DEBUG.)\n"
			"            <classname::function>\n"
			"    --logfile|-l <filename>       : output log messages to file\n"
			"    --logfile-lines|-L <filename> : output log messages to file, with line buffering\n"
			"    --stdout-only                 : writes errors/warnings to stdout instead of stderr\n"
	);
}

int main(int argc, char* argv[])
{
	string file = "";

	Severity console_verbosity = Severity::NOTICE;
	for(int i = 1; i < argc; i++)
	{
		string s(argv[i]);

		if(ParseLoggerArguments(i, argc, argv, console_verbosity))
			continue;

		if(s == "--help")
		{
			help();
			return 0;
		}

		if (s[0] != '-')
		{
			// Must be the file argument
			file = s;
			continue;
		}

		fprintf(stderr, "Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
	}

	if(file.empty())
	{
		fprintf(stderr, "You must specify a file to extract from\n");
		return 1;
	}

	g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(console_verbosity));

	ifstream stream(file.c_str(), ios::binary | ios::in);
	if(!stream.good())
	{
		LogError("Unable to open file!");
		return 1;
	}
	NSIS nsis(std::move(stream));

	// The extraction is pretty simple as the files are packaged

}