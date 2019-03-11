#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>

#include "LeachController.hpp"
#include "BiasReset.hpp"


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



	LeachController _ThisRunControllerInstance;

	/*First, check if the settings file has changed in any way*/
	_ThisRunControllerInstance.CheckForSettingsChange();


	/*Expose*/
	unsigned short *ImageBufferV;
	_ThisRunControllerInstance.ExposeCCD(ExposeSeconds, ImageBufferV);


	int nSkipperR;
	if (_ThisRunControllerInstance.CCDParams.CCDType=="DES")	_ThisRunControllerInstance.CCDParams.nSkipperR=1;
	arc::fits::CArcFitsFile cFits( OutFileName.c_str(), _ThisRunControllerInstance.CCDParams.dRows, _ThisRunControllerInstance.CCDParams.dCols*_ThisRunControllerInstance.CCDParams.nSkipperR );

	cFits.WriteKeyword("CCDType",(void*) _ThisRunControllerInstance.CCDParams.CCDType.c_str(), cFits.FITS_STRING_KEY, "CCD Array technology");
	cFits.WriteKeyword("AMPL",(void*) _ThisRunControllerInstance.CCDParams.AmplifierDirection.c_str(), cFits.FITS_STRING_KEY, "Amplifier direction");
	cFits.WriteKeyword("ASMFile",(void*) _ThisRunControllerInstance.CCDParams.sTimFile.c_str(), cFits.FITS_STRING_KEY, "Timing file used");
	cFits.WriteKeyword("NDCMs",(void*) _ThisRunControllerInstance.CCDParams.nSkipperR, cFits.FITS_STRING_KEY, "NDCMs (1 for DES)");
	cFits.WriteKeyword("ExposeSeconds",(void*) ExposeSeconds, cFits.FITS_STRING_KEY, "Exposure");

	cFits.Write( ImageBufferV );




}

