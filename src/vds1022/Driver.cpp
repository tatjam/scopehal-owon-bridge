#include "Driver.h"

#include <algorithm>
#include <cmath>

#include "../../lib/log/log.h"
#include <string>
#include <cstring>
#include <istream>
#include <fstream>

#include "VDS1022Cmd.h"

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
	auto rsp = send_command<uint8_t>(CMD_GET_MACHINE, 86);
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
	rsp = send_command<uint8_t>(CMD_QUERY_FPGA, 0);
	if(rsp.value == 0)
	{
		LogError("FPGA FLASHING NOT TESTED YET, FLASH FIRMWARE WITH OFFICIAL SOFTWARE!");
		return false;
		//write_firmware_to_fpga();
	}

	load_default_settings();

	return true;
}

void Driver::deinit()
{
	libusb_release_interface(hnd, 0);
	libusb_close(hnd);
	libusb_exit(nullptr);
}


bool Driver::read_flash()
{
	// Read flash command expects 1 byte argument, which is always 1
	send_command_raw<uint8_t>(CMD_READ_FLASH, 1);
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
		auto as_char = static_cast<char>(*ptr);
		ver.push_back(as_char);
	}
	while(*(ptr++) != 0);


	if(ver.size() > 3)
	{
		const int vern = (static_cast<int>(ver[1]) - 48);
		if((vern > 2 && vern <= 9) || ver.rfind("V2.7.0"))
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
	auto frameSize = send_command<uint32_t>(CMD_LOAD_FPGA, static_cast<uint32_t>(contents.size()));
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

void Driver::push_sampling_config(int32_t rate, bool peak_detect)
{
	// Send sample rate (This is always a whole number under our program)
	int32_t timebase = 100000000 / rate;
	send_command<int32_t>(CMD_SET_TIMEBASE, timebase);
	// TODO: Disable roll-mode, not supported for now (we don't use very low sample rates)
	send_command<uint8_t>(CMD_SET_ROLLMODE, 0);
	// Set peak detect as desired
	send_command<uint8_t>(CMD_SET_PEAKMODE, peak_detect ? 1 : 0);
}

void Driver::push_trigger_config(TriggerConfig config)
{
	send_command<uint16_t>(CMD_SET_MULTI, config.kind == TriggerConfig::EXT ? 2 : 0);

	// Send trigger pos in samples

}


Driver::DataReadResult Driver::get_data(AcquiredData& out_ch1, AcquiredData& out_ch2, unsigned int timeout)
{
	uint16_t channel_set = 0x0505;
	send_command_raw<uint16_t>(CMD_GET_DATA, channel_set);

	std::array<uint8_t, 5211 * 2> read_bytes{};
	int read_bytes_num;
	int ret  = libusb_bulk_transfer(hnd, read_ep, read_bytes.data(), read_bytes.size(), &read_bytes_num, timeout);
	if(ret == LIBUSB_ERROR_TIMEOUT)
	{
		return DataReadResult{.kind = DataReadResult::TIMEOUT};
	} else if(ret != 0)
	{
		return DataReadResult{.kind = DataReadResult::ERROR};
	}

	if(read_bytes_num == 5)
	{
		// Not ready
		return DataReadResult{.kind = DataReadResult::NO_DATA};
	}

	if(read_bytes_num < 5211)
	{
		// Something's wrong
		return DataReadResult{.kind = DataReadResult::ERROR};
	}

	printf("Decoded\n");

	return decode_data(read_bytes.data(), read_bytes_num, out_ch1, out_ch2);

}

Driver::DataReadResult Driver::decode_data(uint8_t* raw, int num_bytes, AcquiredData& out_ch1, AcquiredData& out_ch2)
{
	DataReadResult res{};
	res.kind = DataReadResult::OKAY;

	// First byte is channel num
	// 4-byte uint represents time sum
	// 4-byte uint represents period number
	// 2-byte uint represents cursor, starting from right
	// 100 bytes of trigger buffer
	// 5100 bytes of ADC data

	return res;
}

void Driver::load_default_settings()
{
	// Values taken straight from Wireshark dump of OWON,
	// seems to set the oscilloscope in a pretty sane starting
	// state!

	// TODO: Use value from calibration
	send_command<uint16_t>(CMD_SET_PHASEFINE, 0);
	send_command<uint16_t>(CMD_SET_TRIGGER, 0);
	send_command<uint16_t>(CMD_SET_TRG_HOLDOFF_CH1, 0x8002);
	send_command<uint16_t>(CMD_SET_EDGE_LEVEL_CH1, 0xfd07);
	send_command<uint8_t>(CMD_SET_CHANNEL_CH1, 0xa0);
	// TODO: Use value from calibration
	send_command<uint16_t>(CMD_SET_VOLT_GAIN_CH1, 0x021b);
	// TODO: Use value from calibration
	send_command<uint16_t>(CMD_SET_ZERO_OFF_CH1, 0x0576);
	send_command<uint8_t>(CMD_CHL_ON, 0x03);
	send_command<uint8_t>(CMD_SET_CHANNEL_CH1, 0xa0);
	// TODO: Use value from calibration
	send_command<uint16_t>(CMD_SET_VOLT_GAIN_CH2, 0x0218);
	// TODO: Use value from calibration
	send_command<uint16_t>(CMD_SET_ZERO_OFF_CH1, 0x057b);
	send_command<uint16_t>(CMD_SET_DEEPMEMORY, 0x13ec);
	send_command<uint8_t>(CMD_SET_MULTI, 0x0);
	send_command<uint32_t>(CMD_SET_TIMEBASE, 0x00000109);
	send_command<uint8_t>(CMD_SET_PEAKMODE, 0x0);
	send_command<uint8_t>(CMD_SET_ROLLMODE, 0x0);
	send_command<uint16_t>(CMD_SET_PRE_TRG, 0x09f6);
	send_command<uint32_t>(CMD_SET_SUF_TRG, 0x000009f6);
	// An EMPTY cmd is sent here, not needed apparently
	// Device is ready to start acquisition

}

bool Driver::init_findany()
{
	LogNotice("Enumerating USB devices to find scope\n");

	// Enumerate devices to find the oscilloscope
	libusb_device** devices;
	ssize_t dev_count = libusb_get_device_list(nullptr, &devices);
	if(dev_count < 0)
	{
		LogError("Unable to enumerate devices\n");
		return false;
	}

	libusb_device* dev;
	libusb_device_handle* devh = nullptr;
	int devi = 0;

	uint8_t nwrite_ep = 255;
	uint8_t nread_ep = 255;

	while((dev = devices[devi++]) != nullptr)
	{
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if(r < 0) continue;

		if(desc.idVendor == 0x5345 && desc.idProduct == 0x1234)
		{
			LogNotice("Found scope, trying to claim\n");
			r = libusb_open(dev, &devh);
			if(r < 0)
			{
				LogError("Unable to open usb device\n");
				break;
			}

			// Get endpoints, one for writing, one for reading
			struct libusb_config_descriptor* config;
			libusb_get_config_descriptor(dev, 0, &config);
			if(config->bNumInterfaces < 1)
			{
				LogError("No USB interfaces found on device");
				libusb_free_config_descriptor(config);
				break;
			}

			if(config->interface[0].num_altsetting < 1)
			{
				LogError("No USB alt setting found on device");
				libusb_free_config_descriptor(config);
				break;
			}

			const struct libusb_interface_descriptor* iface =
					&config->interface[0].altsetting[0];

			for(size_t i = 0; i < iface->bNumEndpoints; i++)
			{
				uint8_t addr = iface->endpoint[i].bEndpointAddress;
				if((addr & 0x80) == 0 && nwrite_ep == 255)
				{
					// write endpoint
					nwrite_ep = addr;
				}
				if((addr & 0x80) != 0 && nread_ep == 255)
				{
					nread_ep = addr;
				}
			}

			if(nwrite_ep == 255 || nread_ep == 255)
			{
				LogError("Unable to find R/W endpoints");
				libusb_free_config_descriptor(config);
				break;
			}

			libusb_free_config_descriptor(config);

			r = libusb_claim_interface(devh, 0);
			if(r < 0)
			{
				LogError("Unable to clain usb interface\n");
				libusb_close(devh);
				devh = nullptr;
				break;
			}


			// We can now interact with the device at will
			LogNotice("Connection with device initialized\n");
		}

	}

	libusb_free_device_list(devices, 1);

	if(!devh)
	{
		LogError("Unable to find or claim OWON scope\n");
		return false;
	}

	// Start operation
	if(!init(devh, nwrite_ep, nread_ep))
	{
		LogError("Failed initialization, cleaning up\n");
		libusb_release_interface(devh, 0);
		libusb_close(devh);
		return false;
	}

	return true;
}


