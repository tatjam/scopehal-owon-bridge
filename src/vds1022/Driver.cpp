#include "Driver.h"
#include "../../lib/log/log.h"

// Little-endian! LSB at littlest address

template<typename T>
CommandResponse Driver::send_command(uint32_t addr, T data)
{
	std::array<uint8_t, sizeof(T) + 4 + 1> bytes{};
	bytes[0] = addr & 0xFF;
	bytes[1] = (addr & 0xFF00) >> (8 * 1);
	bytes[2] = (addr & 0xFF0000) >> (8 * 2);
	bytes[3] = (addr & 0xFF000000) >> (8 * 3);
	bytes[4] = sizeof(T);
	// write data
	for(int i = 0; i < sizeof(T); i++)
	{
		bytes[5 + i] = (data >> (8 * i)) & 0xFF;
	}
	// Because this transfer is very small, we can just ignore
	// the timeouts and message division that libusb may do
	libusb_bulk_transfer(hnd, write_ep, bytes.data(), bytes.size(), nullptr, 0);

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
	auto rsp = send_command(0x4001, 86);
	if(rsp.value != 1)
	{
		LogError("Incorrect device version\n");
		return false;
	}

	LogNotice("Correct device version detected\n");


	return true;
}

