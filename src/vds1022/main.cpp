#include <libusb.h>
#include <iostream>
#include <string>

#include "Driver.h"
#include "OWONSCPIServer.h"

using namespace std;

#include "../../lib/log/log.h"

void help()
{
	fprintf(stderr,
			"vds1022 [general options] [bridge options] [logger options]\n"
			"\n"
			"  [general options]:\n"
			"    --help                        : this message...\n"
			"\n"
			"  [bridge options]:\n"
			"    --scpi-port                   : set port for scpi, default 5025...\n"
			"    --waveform-port               : set port for waveforms, default 5026...\n"
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
	uint16_t scpiPort = 5025;
	uint16_t waveformPort = 5026;

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
		else
		{
			fprintf(stderr, "Unrecognized command-line argument \"%s\", use --help\n", s.c_str());
		}
	}

	g_log_sinks.emplace(g_log_sinks.begin(), new STDLogSink(console_verbosity));

	int r = libusb_init_context(nullptr, nullptr, 0);
	if(r < 0)
	{
		LogError("Unable to initialize libusb\n");
		return r;
	}

	Driver driver;
	if(!driver.init_findany())
	{
		LogError("Unable to initialize driver\n");
		return -1;
	}

	TriggerConfig tconfig;
	tconfig.kind = TriggerConfig::SINGLE_A;
	tconfig.channel_config[0].condition = TriggerConfig::ChannelConfig::RISE;
	driver.push_trigger_config(tconfig);
	for(int i = 0; i < 1000; i++)
	{
        driver.get_data();
		usleep(100000);
	}

	Socket scpiSocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	Socket waveformSocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);


	scpiSocket.SetReuseaddr(true);
	scpiSocket.Bind(scpiPort);
	scpiSocket.Listen();

	waveformSocket.SetReuseaddr(true);
	waveformSocket.Bind(waveformPort);
	waveformSocket.Listen();


	while(true)
	{
		LogNotice("Waiting for ngscope to connect\n");

		Socket scpiClient = scpiSocket.Accept();
		if(!scpiClient.IsValid()) break;

		Socket dataClient = waveformSocket.Accept();
		if(!dataClient.IsValid()) break;

		if(!dataClient.DisableNagle())
		{
			LogWarning("Failed to disable Nagle, performance may be worse.");
		}

		LogNotice("Connected, starting operation!\n");

		OWONSCPIServer server(scpiClient.Detach(), std::move(dataClient), &driver);

		// Launch the waveform obtainer thread

		server.MainLoop();
	}

	driver.deinit();
	libusb_exit(nullptr);

	return 0;

}