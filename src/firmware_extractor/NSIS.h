#pragma once

#include <istream>
#include <fstream>

// Super simple Nullsoft Installer extractor
// (Based on https://github.com/isra17/nrs/)
class NSIS
{
protected:
	bool SeekFirstHeader(std::ifstream& in);
	bool valid;
public:


	// Fully parses the stream
	NSIS(std::ifstream&& in);
};


