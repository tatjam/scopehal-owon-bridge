#include "Driver.h"
#include "../../lib/log/log.h"
#include <string>

// Little-endian! LSB at littlest address

template<typename T>
void Driver::send_command_raw(uint32_t addr, T data)
{
	std::array<uint8_t, sizeof(T) + 4 + 1> bytes{};
	bytes[0] = addr & 0xFF;
	bytes[1] = (addr & 0xFF00) >> (8 * 1);
	bytes[2] = (addr & 0xFF0000) >> (8 * 2);
	bytes[3] = (addr & 0xFF000000) >> (8 * 3);
	bytes[4] = sizeof(T);
	// write data
	for(size_t i = 0; i < sizeof(T); i++)
	{
		bytes[5 + i] = (data >> (8 * i)) & 0xFF;
	}
	// Because this transfer is very small, we can just ignore
	// the timeouts and message division that libusb may do
	libusb_bulk_transfer(hnd, write_ep, bytes.data(), bytes.size(), nullptr, 0);

}

template<typename T>
CommandResponse Driver::send_command(uint32_t addr, T data)
{
	send_command_raw<T>(addr, data);
	// Now listen for response
	std::array<uint8_t, 5> read_bytes{};
	libusb_bulk_transfer(hnd, read_ep, read_bytes.data(), read_bytes.size(), nullptr, 0);

	CommandResponse out{};
	out.status = read_bytes[0];
	out.value = read_bytes[1];
	out.value |= read_bytes[2] << (8 * 1);
	out.value |= read_bytes[3] << (8 * 2);
	out.value |= read_bytes[4] << (8 * 3);

	return out;
}



bool Driver::init(libusb_device_handle* _hnd, uint8_t _write_ep, uint8_t _read_ep)
{
	this->hnd = _hnd;
	this->write_ep = _write_ep;
	this->read_ep = _read_ep;

	// Check correct device version to avoid possible damage
	auto rsp = send_command<uint8_t>(0x4001, 86);
	if(rsp.value != 1)
	{
		LogError("Incorrect device version\n");
		return false;
	}

	LogNotice("Correct device version detected\n");

	if(!read_flash())
	{
		return false;
	}

	// Check FPGA, we don't flash it! The user must atleast launch
	// the official software once
	rsp = send_command<uint8_t>(0X0223, 0);
	if(rsp.value == 0)
	{
		// TODO: Flash the device (kind of wonky licensing, so maybe not a good idea!)
		LogError("Device is not flashed. Launch official software to do so!\n");
		LogError("Once software has been launched once, the device will remain\\
flashed until it's disconnected from the computer. (Software can be closed)\n");
		LogError("If you are running linux, you can use: https://github.com/florentbr/OWON-VDS1022/tree/master\n");
		return false;
	}

	return true;
}

bool Driver::read_flash()
{
	// Read flash command expects 1 byte argument
	send_command_raw<uint8_t>(0x01b0, 1);
	std::array<uint8_t, 2002> flash{};
	libusb_bulk_transfer(hnd, read_ep, flash.data(), flash.size(), nullptr, 0);

	// Check header
	if(!(
			(flash[0] == 0xAA && flash[1]  == 0x55) ||  // Header 0x55AA
			(flash[0] == 0x55 && flash[1] == 0xAA)))	// Header 0xAA55
	{
		LogError("Invalid flash header version, aborting\n");
		return false;
	}

	// Check version
	if(flash[2] != 2)
	{
		LogError("Invalid flash version, aborting\n");
		return false;
	}

	// Read calibration
	for(int ch = 0; ch < 2; ch++)
	{
		for(int volt = 0; volt < 10; volt++)
		{
			// Gain
			uint8_t gain0 = flash[6 + ch * 20 + volt * 2 + 0];
			uint8_t gain1 = flash[6 + ch * 20 + volt * 2 + 1];
			uint8_t ampl0 = flash[46 + ch * 20 + volt * 2 + 0];
			uint8_t ampl1 = flash[46 + ch * 20 + volt * 2 + 0];
			uint8_t comp0 = flash[86 + ch * 20 + volt * 2 + 0];
			uint8_t comp1 = flash[86 + ch * 20 + volt * 2 + 1];

			uint16_t gain = (uint16_t)gain0 | ((uint16_t)gain1 << 8);
			uint16_t ampl = (uint16_t)ampl0 | ((uint16_t)ampl1 << 8);
			uint16_t comp = (uint16_t)comp0 | ((uint16_t)comp1 << 8);

			calibration[ch].gain[volt] = gain;
			calibration[ch].ampl[volt] = ampl;
			calibration[ch].comp[volt] = comp;
		}
	}

	// Read version to determine implementation quirks
	// (It's a nul terminated C string)
	std::string ver;
	uint8_t* ptr = &flash[207];
	do
	{
		ver.push_back(*ptr);
	}
	while(*(ptr++) != 0);


	if(ver.size() > 3)
	{
		int vern = ((int)ver[1] - 48);
		if(vern > 2 && vern <= 9)
		{
			old_board = false;
		}
		else if(ver.rfind("V2.7.0"))
		{
			old_board = false;
		}
		else
		{
			old_board = true;
		}
	}
	else
	{
		LogError("Bad firmware version: %s", ver.c_str());
		return false;
	}


	return true;
}


