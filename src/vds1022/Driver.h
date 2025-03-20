#pragma once
#include <string>
#include <array>
#include <libusb.h>
#include <vector>

// Reverse engineering from https://github.com/florentbr/OWON-VDS1022/tree/master
// and some performed by myself by inspecting USB packets
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


struct TriggerConfig
{
	enum TriggerKind
	{
		SINGLE_A,
		SINGLE_B,
		ALTERNATE,
		EXT,		// Only supports edge trigger mode!
	};

	struct ChannelConfig
	{
		enum TriggerMode
		{
			EDGE,
			PULSE,
			SLOPE
		};

		enum TriggerCondition
		{
			RISE,	// Only on edge mode
			FALL,	// Only on edge mode
			RISE_GT_REF,	// Trigger on rising, when speed is faster than reference time
			RISE_EQ_REF,	// Trigger on rising, when speed is equal to reference time
			RISE_LT_REF,	// Trigger on rising, when speed is less than reference time
			FALL_GT_REF,	// Trigger on rising, when speed is faster than reference time
			FALL_EQ_REF,	// Trigger on rising, when speed is equal to reference time
			FALL_LT_REF,	// Trigger on rising, when speed is less than reference time
		};

		enum SweepMode
		{
			AUTO,
			NORMAL,
			ONCE,
		};

		TriggerMode mode;
		TriggerCondition condition;
		SweepMode sweep_mode;

		// Trigger level (0 to 1) for all modes. In slope mode, it represents
		// the low value of the slope
		float level;
		// Trigger level (0 to 1) for slope mode, where it represents the
		// high value of the slope
		float level_hi;
		// In slope mode, represents reference time between lo/hi
		// In pulse mode, represents reference pulse width
		float width;
		// Time before next trigger can occur
		float holdoff;
	};

	TriggerKind kind;

	// A, B, and EXT trigger
	ChannelConfig channel_config[3];

};

struct AcquiredData
{
	uint32_t time_sum;
	uint32_t period_num;
	uint32_t cursor;
	std::array<uint8_t, 100> trigger_buf;
	std::array<uint8_t, 5100> samples;
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

	void push_sampling_config(int32_t srate, bool peak_detect);
	void push_trigger_config(TriggerConfig config);

	struct DataReadResult
	{
		enum Kind
		{
			OKAY,
            TIMEOUT,
            ERROR,
            NO_DATA,
		};
		Kind kind;
		bool has_ch1;
		bool has_ch2;
	};
	// timeout in ms, use 0 for no timeout. Returns:
	// 0 if no new data was available (timed-out or error)
	// 1 if data was written to ch1
	// 2 if data was written to ch2
	// 3 if data was written to both
	DataReadResult get_data(AcquiredData& out_ch1, AcquiredData& out_ch2, unsigned int timeout);
	DataReadResult decode_data(uint8_t* raw, int num_bytes, AcquiredData& out_ch1, AcquiredData& out_ch2);

	void load_default_settings();

	bool init_findany();
	bool init(libusb_device_handle* _hnd, uint8_t _write_ep, uint8_t _read_ep);

	void deinit();
};

