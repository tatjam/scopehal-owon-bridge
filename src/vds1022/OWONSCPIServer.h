#pragma once

#include "Driver.h"
#include "../../lib/scpi-server-tools/BridgeSCPIServer.h"
#include <mutex>
#include <atomic>
#include <thread>

class OWONSCPIServer : public BridgeSCPIServer
{
public:
	ZSOCKET scpi_socket;
	Socket waveform_socket;

	OWONSCPIServer(ZSOCKET sock, Socket&& wsock, Driver* driver);
	~OWONSCPIServer() override;

	std::mutex device_mtx;
	std::thread waveform_thread;

	std::atomic<bool> waveform_quit;

	// Acquires waveforms from the device, so main thread can work
	// on communication only (and SCPI commands themselves)
	void waveform_server();

protected:

	Driver* driver;


	std::string GetMake() override;
	std::string GetModel() override;
	std::string GetSerial() override;
	std::string GetFirmwareVersion() override;
	size_t GetAnalogChannelCount() override;
	std::vector<size_t> GetSampleRates() override;
	std::vector<size_t> GetSampleDepths() override;
	void AcquisitionStart(bool oneShot) override;
	void AcquisitionForceTrigger() override;
	void AcquisitionStop() override;
	bool IsTriggerArmed() override;
	void SetChannelEnabled(size_t chIndex, bool enabled) override;
	void SetAnalogCoupling(size_t chIndex, const std::string& coupling) override;
	void SetAnalogRange(size_t chIndex, double range_V) override;
	void SetAnalogOffset(size_t chIndex, double offset_V) override;
	void SetDigitalThreshold(size_t chIndex, double threshold_V) override;
	void SetDigitalHysteresis(size_t chIndex, double hysteresis) override;
	void SetSampleRate(uint64_t rate_hz) override;
	void SetSampleDepth(uint64_t depth) override;
	void SetTriggerDelay(uint64_t delay_fs) override;
	void SetTriggerSource(size_t chIndex) override;
	void SetTriggerLevel(double level_V) override;
	void SetTriggerTypeEdge() override;
	void SetEdgeTriggerEdge(const std::string& edge) override;
	bool GetChannelID(const std::string& subject, size_t& id_out) override;
	ChannelType GetChannelType(size_t channel) override;

	bool OnCommand(const std::string& line, const std::string& subject, const std::string& cmd,
		const std::vector<std::string>& args) override;
	bool OnQuery(const std::string& line, const std::string& subject, const std::string& cmd) override;

	void WorkerThreadFunc(OWONSCPIServer* server);
};
