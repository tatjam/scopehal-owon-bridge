#include <libusb.h>
#include <iostream>
#include <string>

using namespace std;

#include "../../lib/log/log.h"

void help()
{
	fprintf(stderr,
			"vds1022 [general options] [logger options]\n"
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

	LogNotice("Enumerating USB devices to find scope\n");

	int r = libusb_init_context(nullptr, nullptr, 0);
	if(r < 0)
	{
		LogError("Unable to initialize libusb\n");
		return r;
	}

	// Enumerate devices to find the oscilloscope
	libusb_device** devices;
	ssize_t dev_count = libusb_get_device_list(nullptr, &devices);
	if(dev_count < 0)
	{
		LogError("Unable to enumerate devices\n");
		libusb_exit(nullptr);
		return (int)dev_count;
	}

	libusb_device* dev;
	libusb_device_handle* devh = nullptr;
	int i = 0;
	while((dev = devices[i++]) != nullptr)
	{
		struct libusb_device_descriptor desc;
		r = libusb_get_device_descriptor(dev, &desc);
		if(r < 0) continue;

		if(desc.idVendor == 0x5345 && desc.idProduct == 0x1234)
		{
			LogNotice("Found scope, trying to claim\n");
			r = libusb_open(dev, &devh);
			if(r < 0)
			{
				LogError("Unable to open usb device");
				break;
			}

			r = libusb_claim_interface(devh, 0);
			if(r < 0)
			{
				LogError("Unable to clain usb interface");
				libusb_close(devh);
				devh = nullptr;
				break;
			}

			// We can now interact with the device at will

		}

	}

	if(!devh)
	{
		LogError("Unable to find or claim OWON scope\n");
		libusb_free_device_list(devices, 1);
		libusb_exit(nullptr);
		return -1;
	}

	// Make sure device is correct

	// Start operation

	LogNotice("Cleaning up\n");
	libusb_release_interface(devh, 0);
	libusb_close(devh);

	libusb_free_device_list(devices, 1);
	libusb_exit(nullptr);
	return 0;

}