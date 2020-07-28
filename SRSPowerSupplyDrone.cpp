#include "SRSPowerSupplyController.hpp"
#include <iostream>
#include <string>

int main(int argc, char const *argv[])
{
	// Create serial communication with power supply
	auto * srsSupply = new SRSPowerSupplyController("/dev/ttyUSB3");
	sleep(0.5);

	double setVoltage = 25;
	double setVoltageTime = 4;
	printf("Set power supply to %.2f V. Hold for %.1f seconds.\n", setVoltage, setVoltage);
	srsSupply->WritePSVoltage(setVoltage);
	sleep(setVoltageTime);

	printf("Test Ramp up....\n");
	srsSupply->VoltageRamp(0, 75, 10, true);
	sleep(15);

	printf("Test Ramp down....\n");
	srsSupply->VoltageRamp(srsSupply->ReadPSVoltage(), 0, 10, true);


	return 0;
}