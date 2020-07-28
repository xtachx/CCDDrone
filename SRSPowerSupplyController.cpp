#include "SRSPowerSupplyController.hpp"

SRSPowerSupplyController::SRSPowerSupplyController(){
	this->currentVoltage = this->GetPSVoltage();
    this->currentOutputStatus = this->GetP
    printf("Dummy instance of class. No serial communication available");
}

SRSPowerSupplyController::SRSPowerSupplyController(std::string SerialPort) : SerialDevice(SerialPort){
	 /* Set Baud Rate */
        cfsetospeed (&this->tty, (speed_t)B115200);
        cfsetispeed (&this->tty, (speed_t)B115200);

        /* Setting other Port Stuff */
        // Found that the defaults in tty worked best for arduino communication

        /* Flush Port, then applies attributes */
        tcflush( USB, TCIFLUSH );
        if ( tcsetattr ( USB, TCSANOW, &tty ) != 0)
        {
            std::cout << "Error " << errno << " from tcsetattr" << std::endl;
        }


        this->currentVoltage = this->GetPSVoltage();
        this->currentOutputStatus = this->GetPSOutput();
        this->WatchdogFuse = 1;
        this->valveState = 0;
        this->timeBetweenFillState = 0;
        this->overflowVoltage = 0;
        this->timeInCurrentState = 0;
        this->isOverflow = 0;

        printf("SRS Power Supply is now ready to accept instructions.\n");
}

SRSPowerSupplyController::~SRSPowerSupplyController(){
	close(USB);
}

double SRSPowerSupplyController::ReadPSVoltage(){
	std::string srsCmd = "SOUT?";

	this->WriteString(srsCmd);
	std::string srsReturn = this->ReadLine();

	 try {
        this->currentOutputStatus = std::stoi(srsReturn);
    } catch(...){
        printf("Error in ReadPSOutput\n");
    }

    return this->currentOutputStatus;
}

bool SRSPowerSupplyController::ReadPSOutput(){

	std::string srsCmd = "SOUT?";

	this->WriteString(srsCmd);
	std::string srsReturn = this->ReadLine();

	 try {
        this->currentOutputStatus = std::stoi(srsReturn);
    } catch(...){
        printf("Error in ReadPSOutput\n");
    }

    return this->currentOutputStatus;

}

void SRSPowerSupplyController::WritePSVoltage(double voltage){
	std::string srsCmd = "VOLT " + std::to_string(voltage);

	// Write to power supply
	this->WriteString(srsCmd);

	currentVoltage = this->ReadPSVoltage();
}

void SRSPowerSupplyController::WritePSOutput(bool output){
	std::string srsCmd = "SOUT " + std::to_string(output);

	// Write to power supply
	this->WriteString(srsCmd);

	currentOutputStatus = this->ReadPSOuput();
}

void SRSPowerSupplyController::VoltageRamp(double startScanVoltage, double stopScanVoltage, double scanTime, bool display){

	// Define ramp parmaeters
	std::string srsCmd;
    std::srsCmd = "SCAR RANGE" + std::to_string(100);
    this->WriteString(srsCmd);

    // Ramp start voltage
    srsCmd = "SCAB " + std::to_string(startScanVoltage);
    this->WriteString(srsCmd); 

    // Ramp stop voltage
    srsCmd = "SCAE " + std::to_string(stopScanVoltage);
    this->WriteString(srsCmd);

    // Ramp time
    srsCmd = "SCAT " + std::to_string(scanTime);
    this->WriteString(srsCmd);

    // Other options necessary for ramp
    this->WritePSOutput(true);
    srsCmd = "SCAD ON";
    this->WriteString(srsCmd);
    srsCmd = "SCAA 1";
    this->WriteString(srsCmd);

}
