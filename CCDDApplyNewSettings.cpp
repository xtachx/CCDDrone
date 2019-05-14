#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>

#include "LeachController.hpp"
#include <chrono>
#include <thread>


// ------------------------------------------------------
//  Main program
// ------------------------------------------------------
int main( int argc, char **argv )
{


    std::cout << "This code apply new settings on the the clock and bias voltages.\n";

	std::string configFileName;

	/*Now get the args*/
	if (argc<2) {
		std::cout << "Default usage: ./CCDDApplyNewSettings <config file>. ";
		std::cout << "No config file was specified. Using config/Config.ini\n";

		configFileName = "config/Config.ini";
	} else {
		configFileName = argv[2];
	}


	LeachController _ThisRunControllerInstance(configFileName);

	std::cout<<"Checking for new settings and loading them.\n";
	//_ThisRunControllerInstance.LoadCCDSettingsFresh();
	bool config, sequencer;
	int _CCDSettingsStatus = _ThisRunControllerInstance.LoadAndCheckForSettingsChange(config, sequencer);

    if (sequencer){
		std::cout<<"Applying new sequencer.\n";
		_ThisRunControllerInstance.ApplyNewSequencer(_ThisRunControllerInstance.CCDParams.sTimFile);
	}

	/*Apply biases and clocks*/
	std::cout<<"Applying biases and clocks.\n";
	_ThisRunControllerInstance.ApplyAllCCDBasic();
	_ThisRunControllerInstance.ApplyAllBiasVoltages();
	_ThisRunControllerInstance.ApplyAllCCDClocks();



	_ThisRunControllerInstance.CopyOldAndStoreFileHashes();



	std::cout<<"Set IDLE clocks to ON.\n";
    _ThisRunControllerInstance.IdleClockToggle();


    std::cout<<"New settings have been uploaded to the Leach system.\n";

	return 0;
}

