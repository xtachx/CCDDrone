#include "SRSPowerSupplyController.hpp"

SRSPowerSupplyController::SRSPowerSupplyController(){
	this->currentVoltage = 0;
    this->currentOutputStatus = 0;
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


        this->currentOutputStatus = 0;
        this->WritePSOutput(this->currentOutputStatus);
        this->currentVoltage = this->ReadPSVoltage();
        this->WriteString("RANGE100");


        printf("SRS Power Supply is now ready to accept instructions.\n");
}

SRSPowerSupplyController::~SRSPowerSupplyController(){
    this->currentOutputStatus = 0;
    this->WritePSOutput(this->currentOutputStatus);
	close(USB);
}

double SRSPowerSupplyController::ReadPSVoltage(){
	std::string srsCmd = "VOLT?\n";

	this->WriteString(srsCmd);
	std::string srsReturn = this->ReadLine();

	 try {
        this->currentVoltage = std::stoi(srsReturn);
    } catch(...){
        printf("Error in ReadPSOutput\n");
    }

    return this->currentVoltage;
}

bool SRSPowerSupplyController::ReadPSOutput(){

	std::string srsCmd = "SOUT?\n";

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
	std::string srsCmd = "VOLT " + std::to_string(voltage) +"\n";

	// Write to power supply
	this->WriteString(srsCmd);

	currentVoltage = this->ReadPSVoltage();
}

void SRSPowerSupplyController::WritePSOutput(bool output){
	std::string srsCmd = "SOUT " + std::to_string(output) + "\n";

	// Write to power supply
	this->WriteString(srsCmd);

	currentOutputStatus = this->ReadPSOutput();
}

void SRSPowerSupplyController::VoltageRamp(double startScanVoltage, double stopScanVoltage, double scanTime, bool display){

	// Define ramp parmaeters
	std::string srsCmd;
    srsCmd = "SCAR RANGE" + std::to_string(100) + "\n";
    this->WriteString(srsCmd);

    // Ramp start voltage
    srsCmd = "SCAB " + std::to_string(startScanVoltage) + "\n";
    this->WriteString(srsCmd); 

    // Ramp stop voltage
    srsCmd = "SCAE " + std::to_string(stopScanVoltage) + "\n";
    this->WriteString(srsCmd);

    // Ramp time
    srsCmd = "SCAT " + std::to_string(scanTime) + "\n";
    this->WriteString(srsCmd);

    // Other options necessary for ramp
    this->WritePSOutput(true);
    srsCmd = "SCAD ON\n";
    this->WriteString(srsCmd);
    srsCmd = "SCAA 1\n";
    this->WriteString(srsCmd);
    srsCmd = "*TRG\n";
    this->WriteString(srsCmd);

}
