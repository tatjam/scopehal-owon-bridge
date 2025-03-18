#pragma once

// READ_FLASH
// Takes a byte argument (which must be 1) and returns 2002 bytes with the
// flash information. As given by florentbr:
// #     Offset  Size        Field             Value
//     0       uint16      Flash header      0x55AA or 0xAA55
//     2       uint32      Flash version     2
//     6       uint16[10]  CH1 Gain          for [ 5mV 10mV 20mV 50mV 100mv 200mv 500mv 1v 2v 5v ]
//     26      uint16[10]  CH2 Gain          for [ 5mV 10mV 20mV 50mV 100mv 200mv 500mv 1v 2v 5v ]
//     46      uint16[10]  CH1 Amplitude     for [ 5mV 10mV 20mV 50mV 100mv 200mv 500mv 1v 2v 5v ]
//     66      uint16[10]  CH2 Amplitude     for [ 5mV 10mV 20mV 50mV 100mv 200mv 500mv 1v 2v 5v ]
//     86      uint16[10]  CH1 Compensation  for [ 5mV 10mV 20mV 50mV 100mv 200mv 500mv 1v 2v 5v ]
//     106     uint16[10]  CH2 Compensation  for [ 5mV 10mV 20mV 50mV 100mv 200mv 500mv 1v 2v 5v ]
//     206     byte        OEM               0 or 1
//     207     char*       Device version    null terminated string - ASCII encoded
//             char*       Device serial     null terminated string - ASCII encoded
//             byte[100]   Localizations     0 or 1 for zh_CN, zh_TW, en, fr, es, ru, de, pl, pt_BR, it, ja, ko_KR
//             uint16      Phase fine        0-255  ???
// The gains, amplitude and compensations are calibration values
// Phase fine is unknown function but needs to be written back to the device
#define CMD_READ_FLASH 0x01b0
// Overwrites device flash with a given byte stream, not recommended to be used
// Takes a byte, which must be 1, and afterwards expects the 2002 bytes of flash
// Returns CommandResponse
#define CMD_WRITE_FLASH 0x01a0
// Queries whether FPGA is flashed. Takes a byte which must be 0, and returns a CommandResponse
// with value '0' if flashed and '1' if not flashed
#define CMD_QUERY_FPGA 0x0223
// Takes a 32 bit int which is the length of the FPGA binary file, and returns a CommandResponse
// which specifies in its value the 'frame_size' which is how many bytes should we send at once.
// These bytes contain:
// - Header: a 32-bit int which specifies the index of the frame being sent
// - frame_size - 4 bytes of payload information from the FPGA binary
// After each bulk transfer of these bytes, the device replies with a CommandResponse, which
// contains in its value the index written (and success info of course)
#define CMD_LOAD_FPGA 0x4000
// No idea what this does
#define CMD_EMPTY 0x010c
// Returns a CommandResponse with value:
// 0: error
// 1: VDS1022
// 2: VDS2052
#define CMD_GET_MACHINE 0x4001
// Takes two 8 bit numbers which contain:
// As given by florentbr:
//	channel 1 state ( OFF:0x04 ON:0x05 )
//	channel 2 state ( OFF:0x04 ON:0x05 )
// Returns bulk data, as given by florentbr:
//   Receive 5 bytes starting with 'E' if not ready
//   Receive 5211 bytes for channel 1 if ON and ready
//     Offset  Size  Field
//     0       1     channel (CH1=0x00 CH2=0x01)
//     1       4     time_sum (used by frequency meter)
//     5       4     period_num (used by frequency meter)
//     9       2     cursor (samples count from the right)
//     11      100   ADC trigger buffer (only used with small time bases)
//     111     5100  ADC buffer ( 50 pre + 5000 samples + 50 post )
//   Receive 5211 bytes for channel 2 if ON and ready
#define CMD_GET_DATA 0x1000
// Takes a byte which must be 0 and returns an int which contains
// in its lowest two bits channel 0 and channel 1 state, or if using
// external trigger, both bits set to ext trigger state
#define CMD_GET_TRIGGERED 0x01
// No idea what this one does
#define CMD_GET_VIDEOTRGD 0x02
// Takes a 8-bit number which is:
// 0: TRIGGER OUT
// 1: PASS-FAIL (Unused)
// 2: TRIGGER IN
#define CMD_SET_MULTI 0x06
// Takes a byte which is 0 if peak detect is off, and 1 if it's on
// Once peak detect is on, samples are interleaved between min and max
// (Thus we get half the samples!)
#define CMD_SET_PEAKMODE 0x09
// Takes a byte which is 0 if roll mode is off, and 1 if it's on
// Roll mode is just the scope sending samples "real-time" instead of acquiring in
// bulk and sending. Of course, this works only on low sample rates.
#define CMD_SET_ROLLMODE 0x0a
// Takes a byte which represents in its lowest bits the state for channel 1 and 2
// (Apparently not needed?)
#define CMD_CHL_ON 0x0b
// Takes a byte which must be 0x3 to trigger
#define CMD_FORCETRG 0x0c
// Takes a 16-bit number which is phasefine from the flash
// (Observed to be 0 in my machine)
#define CMD_SET_PHASEFINE 0x18
// Takes a 16-bit number which determines the trigger config. If setting up an alternate trigger
// two messages must be sent
// bit  0 = trigger from channel (0) or from EXT (1)
// bit  15 = trigger from single channel (0) or alternate both (1)
// ONLY WHEN SETTING UP ALTERNATE CHANNEL:
//		bit  14: alternate channel
//		bits 13,8: alternate channel trigger mode
// ONLY WHEN SETTING UP MAIN CHANNEL:
//		bit 13: channel
//		bits 8,14:
// EDGE MODE ONLY:
//		bit 9: AC couping (0) (always?)
//		bit 10,11: (only works if non-alternate) sweep mode (0: auto, 1: normal, 2: once)
//		bit 12: 0: rise 1: fall
// PULSE OR SLOPE MODES ONLY:
//		bits 5, 6, 7: condition (0: Rise>, 1: Rise=, 2:Rise<, 3:Fall>, 4:Fall=, 5:Fall<
//		bits 10,11: (only works if non-alternate) sweep mode (same as before)
// The other bits set to 0
#define CMD_SET_TRIGGER 0x24
#define CMD_SET_VIDEOLINE 0x32
// Takes a 32-bit number which indicates divider to use with respect to 100Mhz for sampling
#define CMD_SET_TIMEBASE 0x52
// Takes a 32-bit number which is post-trigger size in order to setup trigger point (in samples)
#define CMD_SET_SUF_TRG 0x56
// Takes a 16-bit number which is pre-trigger size in order to setup trigger point (in samples)
#define CMD_SET_PRE_TRG 0x5a
// Takes a 16-bit number which is how many samples to store
// (Use 5100 which is the real memory size)
#define CMD_SET_DEEPMEMORY 0x5c
#define CMD_SET_RUNSTOP 0x61
// If this doesn't return 0 we must wait before trying to acquire data
#define CMD_GET_DATAFINISHED 0x7a
#define CMD_GET_STOPPED 0xb1
// Configure channel 1
// bit 0:	0
// bit 1:	0 input atten off, 1 input atten on
// bit 2,3:	bandwidth limit (0)
// bit 4:	0
// bit 5,6:	coupling (0: DC, 1: AC, 2: GND)
// bit 7:	0 channel off 1 channel on
#define CMD_SET_CHANNEL_CH1 0x0111
// Same as before but for channel 2
#define CMD_SET_CHANNEL_CH2 0x0110
// Send calibration value
#define CMD_SET_ZERO_OFF_CH1 0x010a
// Send calibration value
#define CMD_SET_ZERO_OFF_CH2 0x0108
// Send calibration value
#define CMD_SET_VOLT_GAIN_CH1 0x0116
// Send calibration value
#define CMD_SET_VOLT_GAIN_CH2 0x0114
#define CMD_SET_SLOPE_THRED_CH1 0x10
#define CMD_SET_SLOPE_THRED_CH2 0x12
// Takes a 16-bit number (really two 8 bit signed numbers)
// which indicate high and low trigger level
#define CMD_SET_EDGE_LEVEL_CH1 0x2e
// Takes a 16-bit number (really two 8 bit numbers)
// which indicate high and low trigger level
#define CMD_SET_EDGE_LEVEL_CH2 0x30
// Takes a 16-bit number which is holdoff for channel 1
// in 10th of ns, 10 bits mantissa and base 10 exponent in rem bits
#define CMD_SET_TRG_HOLDOFF_CH1 0x26
// Takes a 16-bit number which is holdoff for channel 2
// in 10th of ns, 10 bits mantissa and base 10 exponent in rem bits
#define CMD_SET_TRG_HOLDOFF_CH2 0x2a

// Only on FPGA <= V2
#define CMD_SET_TRG_CDT_EQU_H_CH1 0x32
#define CMD_SET_TRG_CDT_EQU_H_CH2 0x3a
#define CMD_SET_TRG_CDT_EQU_L_CH1 0x36
#define CMD_SET_TRG_CDT_EQU_L_CH2 0x3e

// On all versions
#define CMD_SET_TRG_CDT_GL_CH1 0x42
#define CMD_SET_TRG_CDT_GL_CH2 0x46

// Only on FPGA >= V3
#define CMD_SET_TRG_CDT_HL_CH1 0x44
#define CMD_SET_TRG_CDT_HL_CH2 0x48

// On all versions
#define CMD_SET_FREQREF_CH1 0x4a
#define CMD_SET_FREQREF_CH2 0x4b
