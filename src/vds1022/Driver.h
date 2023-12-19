#pragma once
#include <array>
#include <libusb.h>

// Reverse engineering from https://github.com/florentbr/OWON-VDS1022/tree/master
// Everything is little endian,
// SO THE CODE WON'T WORK ON BIG ENDIAN MACHINES JUST YET!

struct CommandResponse
{
	uint8_t status; // ASCII '\0', 'D', 'E', 'G', 'S' or 'V'
	uint32_t value;
};

class Driver
{
private:

	libusb_device_handle* hnd;
	uint8_t write_ep;
	uint8_t read_ep;

	// T may be uint8_t, uint16_t or uint32_t
	template<typename T>
	CommandResponse send_command(uint32_t addr, T data);
public:

	bool init(libusb_device_handle* _hnd, uint8_t _write_ep, uint8_t _read_ep);

};
