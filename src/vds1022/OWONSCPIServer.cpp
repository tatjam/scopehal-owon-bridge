#include "OWONSCPIServer.h"

#include <log.h>

OWONSCPIServer::OWONSCPIServer(ZSOCKET sock, Socket&& wsock, Driver* driver)
:	BridgeSCPIServer(sock)
,	waveform_socket(wsock)
,	driver(driver)
{
	scpi_socket = sock;
}

OWONSCPIServer::~OWONSCPIServer()
{
}

std::string OWONSCPIServer::GetMake()
{
	return "OWON";
}

std::string OWONSCPIServer::GetModel()
{
	return "VDS1022";
}

std::string OWONSCPIServer::GetSerial()
{
	return "placeholder";
}

std::string OWONSCPIServer::GetFirmwareVersion()
{
	return "placeholder";
}

size_t OWONSCPIServer::GetAnalogChannelCount()
{
	return 2;
}

std::vector<size_t> OWONSCPIServer::GetSampleRates()
{
	// TODO: There may be a way to obtain it from device
	std::vector<size_t> ret;
	ret.push_back(25);
	ret.push_back(50);
	ret.push_back(125);
	ret.push_back(250);
	ret.push_back(500);
	ret.push_back(1250);
	ret.push_back(2500);
	ret.push_back(5000);
	ret.push_back(12500);
	ret.push_back(25000);
	ret.push_back(50000);
	ret.push_back(125000);
	ret.push_back(250000);
	ret.push_back(1250000);
	ret.push_back(2500000);
	ret.push_back(5000000);
	ret.push_back(12500000);
	ret.push_back(25000000);
	ret.push_back(50000000);
	ret.push_back(100000000);

	return ret;
}

std::vector<size_t> OWONSCPIServer::GetSampleDepths()
{
	std::vector<size_t> ret;
	ret.push_back(5000);

	return ret;
}

void OWONSCPIServer::AcquisitionStart(bool oneShot)
{
}

void OWONSCPIServer::AcquisitionForceTrigger()
{
}

void OWONSCPIServer::AcquisitionStop()
{
}

bool OWONSCPIServer::IsTriggerArmed()
{
	return false;
}

void OWONSCPIServer::SetChannelEnabled(size_t chIndex, bool enabled)
{
}

void OWONSCPIServer::SetAnalogCoupling(size_t chIndex, const std::string& coupling)
{
}

void OWONSCPIServer::SetAnalogRange(size_t chIndex, double range_V)
{
}

void OWONSCPIServer::SetAnalogOffset(size_t chIndex, double offset_V)
{
}

void OWONSCPIServer::SetDigitalThreshold(size_t chIndex, double threshold_V)
{
}

void OWONSCPIServer::SetDigitalHysteresis(size_t chIndex, double hysteresis)
{
}

void OWONSCPIServer::SetSampleRate(uint64_t rate_hz)
{
}

void OWONSCPIServer::SetSampleDepth(uint64_t depth)
{
}

void OWONSCPIServer::SetTriggerDelay(uint64_t delay_fs)
{
}

void OWONSCPIServer::SetTriggerSource(size_t chIndex)
{
}

void OWONSCPIServer::SetTriggerLevel(double level_V)
{
}

void OWONSCPIServer::SetTriggerTypeEdge()
{
}

void OWONSCPIServer::SetEdgeTriggerEdge(const std::string& edge)
{
}

bool OWONSCPIServer::GetChannelID(const std::string& subject, size_t& id_out)
{
	return false;
}

BridgeSCPIServer::ChannelType OWONSCPIServer::GetChannelType(size_t channel)
{
	return CH_ANALOG;
}


bool OWONSCPIServer::OnQuery(const std::string& line, const std::string& subject, const std::string& cmd)
{
	if(BridgeSCPIServer::OnQuery(line, subject, cmd))
	{
		return true;
	}

	return false;
}

void OWONSCPIServer::WorkerThreadFunc(OWONSCPIServer* server)
{
}

bool OWONSCPIServer::OnCommand(const std::string& line, const std::string& subject, const std::string& cmd,
                               const std::vector<std::string>& args)
{
	if(BridgeSCPIServer::OnCommand(line, subject, cmd, args))
	{
		return true;
	}

	return false;
}
