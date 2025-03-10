#include "NSIS.h"
#include "pocketlzma.hpp"
#include <cstdint>
#include <log.h>


// TODO: This assumes endianess of host platform is little endian
NSIS::NSIS(std::ifstream&& in)
{
	valid = false;

	valid |= SeekFirstHeader(in);
	if(!valid) return;

	// After 'NullsoftInst' comes 2 32-bit ints, which indicate two sizes
	char buf32[4], num2[4];
	int32_t uSize, cSize;

	in.read(&buf32[0], 4);
	std::memcpy(&uSize, buf32, 4);
	in.read(&buf32[0], 4);
	std::memcpy(&cSize, buf32, 4);

	int32_t dataSize;

	in.read(&buf32[0], 4);
	std::memcpy(&dataSize, buf32, 4);

	// TODO: We only support non-solid LZMA!
	char buf8;
	in.read(&buf8, 1);
	if(buf8 != 0x5D){ valid = false; return; }
	in.read(&buf8, 1);
	if(buf8 != 0x00){ valid = false; return; }
	in.read(&buf8, 1);
	if(buf8 != 0x00){ valid = false; return; }
	in.read(&buf8, 1);
	in.read(&buf8, 1);
	if(buf8 != 0x00){ valid = false; return; }
	in.read(&buf8, 1);
	if(buf8 & 0x80 != 0){ valid = false; return; }

	// Solid LZMA. Data starts on 0x5D byte, which we skipped over
	in.seekg(-6, std::ios_base::seekdir::_S_cur);

	// not sure why this is needed, taken from nrs source code
	dataSize &= 0x7ffffffff;

}

bool NSIS::SeekFirstHeader(std::ifstream& in)
{
	in.seekg(0);
	// Signature is 0xDEADBEEF (little endian) followed by "NullsoftInst" (16 bytes)
	char signature[16] = {
		char(0xEF), char(0xBE), char(0xAD), char(0xDE),
		'N', 'u', 'l', 'l', 's', 'o', 'f', 't', 'I', 'n', 's', 't'};
	while(!in.eof())
	{
		char readsign[16];

		if(!in.read(&readsign[0], 16))
		{
			break;
		}

		if(std::equal(std::begin(readsign), std::end(readsign), std::begin(signature)))
		{
			// We return seeked to end of header "NullsoftInst"
			return true;
		}

		// Seek back to 1 byte of offset, not 16 as read would seek
		in.seekg(-15, std::ios_base::seekdir::_S_cur);
	}

	return false;
}
