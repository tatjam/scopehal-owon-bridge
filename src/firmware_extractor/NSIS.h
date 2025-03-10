#pragma once

#include <istream>

using namespace std;

// Super simple Nullsoft Installer extractor
class NSIS
{
public:


	// Fully parses the stream
	NSIS(istream in);
};


