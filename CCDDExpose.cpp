#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <chrono>

#include "LeachController.hpp"


#define USAGE( x ) \
            ( std::cout << std::endl << "Usage: ./" << x << " [exp time (s) : Default=5] [Output file name: Default Image.fits]" << std::endl)



// ------------------------------------------------------
//  Main program
// ------------------------------------------------------
int main( int argc, char **argv )
{

    /*Now get the args*/
	if (argc<2) {
		std::cout << "Please specify an exposure value!\n";
		USAGE(argv[0]);
		exit( EXIT_FAILURE );
	}

	int ExposeSeconds;
	try {
		ExposeSeconds = atoi(argv[1]);
	} catch (...)
	{
		ExposeSeconds = 5;
		std::cout << "Exposure was not set correctly. Using a default value of 5 seconds.";
	}

	std::string OutFileName;
	try {
		OutFileName = argv[2];
	} catch (...)
	{
		OutFileName = "Image.fits";
		std::cout << "Output file name is set to Image.fits since no name was provided.";
	}



	LeachController _ThisRunControllerInstance("config/Config.ini");

	/*At the start of the program, log the time*/
    _ThisRunControllerInstance.ClockTimers.ProgramStart = std::chrono::system_clock::now();

	/*Check if the settings file has changed in any way*/
	bool config, sequencer;
	int _CCDSettingsStatus = _ThisRunControllerInstance.LoadAndCheckForSettingsChange(config, sequencer);

    if (_CCDSettingsStatus == 0){

        _ThisRunControllerInstance.CCDParams.fExpTime = ExposeSeconds;
        if (_ThisRunControllerInstance.CCDParams.CCDType=="DES")	_ThisRunControllerInstance.CCDParams.nSkipperR=1;


        /*Reset clock timers in case this is a multi exposure*/
        _ThisRunControllerInstance.ClockTimers.isReadout = false;
        _ThisRunControllerInstance.ClockTimers.isExp = false;
        _ThisRunControllerInstance.ClockTimers.rClockCounter = 0;


        /*Expose*/
        unsigned short *ImageBufferV;
        _ThisRunControllerInstance.PrepareAndExposeCCD(ExposeSeconds, ImageBufferV);

        /*Save FITS*/
        _ThisRunControllerInstance.SaveFits(OutFileName);
    } else {
        if (config) std::cout<<"Error: The config file has changed but the new settings were not uploaded.\n";
        if (sequencer) std::cout<<"Error: The sequencer has changed but it was not uploaded.\n";
        std::cout<<"CCD was not exposed and an image was not taken. Please resolve the conflicts in the config section first.\n";
    }


}

