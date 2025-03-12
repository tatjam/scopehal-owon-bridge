#include "Driver.h"

#include <cmath>

#include "../../lib/log/log.h"
#include <string>
#include <cstring>
#include <istream>
#include <fstream>

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
	return receive_response();
}

CommandResponse Driver::receive_response() const
{
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

	// Query FPGA command,
	rsp = send_command<uint8_t>(0X0223, 0);
	if(rsp.value == 0)
	{
		LogError("FPGA FLASHING NOT TESTED YET, FLASH FIRMWARE WITH OFFICIAL SOFTWARE!");
		return false;
		//write_firmware_to_fpga();
	}

	return true;
}


bool Driver::read_flash()
{
	// Read flash command expects 1 byte argument, which is always 1
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
		auto as_byte = static_cast<uint8_t>(*ptr);
		ver.push_back(as_byte);
	}
	while(*(ptr++) != 0);


	if(ver.size() > 3)
	{
		const int vern = (static_cast<int>(ver[1]) - 48);
		if(vern > 2 && vern <= 9 || ver.rfind("V2.7.0"))
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

bool Driver::write_firmware_to_fpga()
{
	std::string filename = "fpga/VDS1022_FPGAV";
	// Append version
	filename += dev_version;
	filename += ".bin";

	std::ifstream file(filename, std::ios::binary | std::ios::in);
	if(!file.good())
	{
		return false;
	}

	std::vector<uint8_t> contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// This returns how big should we send each chunk to the FPGA, including a 32-bit header.
	// (Thus we send frameSize - 1 byte chunks of the firmware)
	auto frameSize = send_command<uint32_t>(0x4000, static_cast<uint32_t>(contents.size()));
	if(frameSize.value == 0)
	{
		return false;
	}

	uint32_t payloadSize = frameSize.value - 4;
	auto frameCount = static_cast<uint32_t>(
		std::ceil(static_cast<double>(contents.size()) / static_cast<double>(payloadSize)));

	std::vector<uint8_t> buffer;
	buffer.resize(frameSize.value);

	for(uint32_t i = 0; i < frameCount; i++)
	{
		uint32_t b = i * payloadSize;
		std::memcpy(buffer.data(), &i, 4);
		for(uint32_t j = 0; j < payloadSize; j++)
		{
			buffer[j + 4] = contents[b + j];
		}
		// We first write i (as the header) and then the payloadSize bytes
		libusb_bulk_transfer(hnd, write_ep, buffer.data(), static_cast<int>(frameSize.value), nullptr, 0);

		CommandResponse resp = receive_response();
		if(resp.status != 'S')
		{
			return false;
		}
		if(resp.value != i)
		{
			return false;
		}
	}

	return true;
}


