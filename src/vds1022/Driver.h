#pragma once
#include <string>
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

// 5mV 10mV 20mV 50mV 100mV 200mV 500mV 1V 2V 5V
struct Calibration
{
	std::array<uint16_t, 10> gain;
	std::array<uint16_t, 10> ampl;
	std::array<uint16_t, 10> comp;
};

class Driver
{
protected:

	bool old_board;

	libusb_device_handle* hnd;
	uint8_t write_ep;
	uint8_t read_ep;

	std::string dev_version;

	Calibration calibration[2];

	// T may be uint8_t, uint16_t or uint32_t
	// BEWARE! The type of T is vital for the command success!
	template<typename T>
	CommandResponse send_command(uint32_t addr, T data);

	// Sends command but doesn't wait for reply, for non standard commands
	template<typename T>
	void send_command_raw(uint32_t addr, T data);

	CommandResponse receive_response() const;

	// Obtains current oscilloscope calibration, and returns
	// if everything is safe to use
	bool read_flash();
	bool write_firmware_to_fpga();

public:

	bool init_findany();
	bool init(libusb_device_handle* _hnd, uint8_t _write_ep, uint8_t _read_ep);

	void deinit();
};

