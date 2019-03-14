#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>

#include "LeachController.hpp"


#define USAGE( x ) \
            ( std::cout << std::endl << "Usage: ./" << x << " [exp time (s) : Default=5] [Output file name: Default Image.fits]" << std::endl)



// ------------------------------------------------------
//  Main program
// ------------------------------------------------------
int main( int argc, char **argv )
{

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



	LeachController _ThisRunControllerInstance("config/FirstTest.ini");

	/*First, check if the settings file has changed in any way*/
	_ThisRunControllerInstance.LoadAndCheckForSettingsChange();


	/*Expose*/
	unsigned short *ImageBufferV;
	_ThisRunControllerInstance.ExposeCCD(ExposeSeconds, ImageBufferV);


	int nSkipperR;
	if (_ThisRunControllerInstance.CCDParams.CCDType=="DES")	_ThisRunControllerInstance.CCDParams.nSkipperR=1;

	_ThisRunControllerInstance.SaveFits(OutFileName);




}

