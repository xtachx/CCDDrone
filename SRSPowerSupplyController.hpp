#ifndef CCDDRONE_SRSPOWERSUPPLYCONTROLLER_HPP
#define CCDDRONE_SRSPOWERSUPPLYCONTROLLER_HPP

#include "SerialDeviceT.hpp"
#include <iostream>

class SRSPowerSupplyController : public SerialDevice
{
public:
	SRSPowerSupplyController();
	SRSPowerSupplyController(std::string);
	~SRSPowerSupplyController();

	// Get PS Values
	double ReadPSVoltage();
	bool ReadPSOutput();

	// Set PS Values
	void WritePSVoltage(double voltage);
	void WritePSOutput(bool output);


	void VoltageRamp(double startScanVoltage, double stopScanVoltage, double scanTime, bool display);

	double currentVoltage;
	bool currentOutputStatus;
};
#endif //CCDDRONE_SRSPOWERSUPPLYCONTROLLER_HPP