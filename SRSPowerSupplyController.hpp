#ifndef CCDDRONE_SRSPOWERSUPPLYCONTROLLER_HPP
#define CCDDRONE_SRSPOWERSUPPLYCONTROLLER_HPP

/*For Serial IO*/
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <unistd.h>

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