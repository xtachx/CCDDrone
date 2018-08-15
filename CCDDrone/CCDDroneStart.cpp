// +-----------------------------------------------------------------------------------+
// |  File:  ArcAPI35Ex_1.cpp                                                          |
// +-----------------------------------------------------------------------------------+
// |  Description:  This file demonstrates a simple use of the ARC API 3.5 for both    |
// |  the PCI and PCIe interfaces. The first device found is used to setup an attached |
// |  controller and take an exposure.                                                 |
// |                                                                                   |
// |  Author:       Scott Streit @ Astronomical Research Cameras, Inc.                 |
// |  Date:         March 26, 2012                                                     |
// +-----------------------------------------------------------------------------------+

#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>

#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CArcFitsFile.h"
#include "CExpIFace.h"
#include "ArcDefs.h"

#include "BiasReset.hpp"

using namespace std;
using namespace arc;
using namespace arc::device;
using namespace arc::deinterlace;
using namespace arc::fits;


#define USAGE( x ) \
            ( cout << endl << "Usage: " << x \
                        << " [PCI | PCIe] [options]" << endl << endl \
                        << "\toptions:" << endl \
                        << "\t---------------------------------------" << endl \
                        << "\t-f [DSP lod filename : Default=tim.lod] " << endl \
                        << "\t-e [exp time (s) : Default=0.5]" << endl \
                        << "\t-r [rows : Default=1024] " << endl \
                        << "\t-c [cols : Default=1200]" << endl \
                        << "\t-d [deint alg : Default=CDeinterlace::DEINTERLACE_NONE]" \
                        << endl << endl << "\tDeinterlace Values:" \
                        << "\n\t---------------------------------------" \
                        << "\n\t0: None\n\t1: Parallel\n\t2: Serial" \
                        << "\n\t3: CCD Quad\n\t4: IR Quad\n\t5: CDS IR QUAD" \
                        << "\n\t6: Hawaii RG" \
                        << "\n\t7: STA1600" << endl << endl )





// ------------------------------------------------------
//  Function prototypes
// ------------------------------------------------------
std::string SetDots( const char *cStr );


// ------------------------------------------------------
//  Exposure Callback Class
// ------------------------------------------------------
class CExposeListener : public CExpIFace
{
      void ExposeCallback( float fElapsedTime )
      {
            cout << "Elapsed Time: " << fElapsedTime << endl;
}
      void ReadCallback( int dPixelCount )
      {
            cout << "Pixel Count: " << dPixelCount << endl;
} };


// ------------------------------------------------------
//  Main program
// ------------------------------------------------------
int main( int argc, char **argv )
{
	std::string sTimFile		= "tim.lod";
	float       fExpTime		= 0.5;
	int			dRows			= 1024;
	int	        dCols			= 1200;
	int         nSkipperR           = 1;
	int	        dDeintAlg		= CArcDeinterlace::DEINTERLACE_NONE;
	bool        bAbort			= false;

	CExposeListener cExposeListener;

	//
	// Set host device
	//
		if ( argc < 2 )
	{
		  cout << "Error: Invalid number of minimum parameters!" << endl;
		  USAGE( argv[ 0 ] );
		  cout << endl;
		  exit( EXIT_FAILURE );
	}

	string sDev = argv[ 1 ];

	if ( sDev.compare( "PCIe" ) != 0 && sDev.compare( "PCI" ) != 0 )
	{
		  cout << "Error: Invalid device parameter: " << sDev << endl;
		  USAGE( argv[ 0 ] );
		  cout << endl;
		  exit( EXIT_FAILURE );
	}



	//
	// Handle program arguments
	//
	for ( int i=2; i<argc; i++ )
	{
		  std::string sArgv = argv[ i ];

		  if ( sArgv.compare( "-f" ) == 0 && argc >= ( i + 1 ) )
		  {
				sTimFile = argv[ i + 1 ];
				cout << "Timing File Set: " << sTimFile << endl;
		  }

		  else if ( sArgv.compare( "-e" ) == 0 && argc >= ( i + 1 ) )
		  {
				fExpTime = float( atof( argv[ i + 1 ] ) );
				cout << "Exposure Time Set: " << fExpTime << endl;
		  }

		  else if ( sArgv.compare( "-r" ) == 0 && argc >= ( i + 1 ) )
		  {
				dRows = atoi( argv[ i + 1 ] );
				cout << "Number Of Rows Set: " << dRows << endl;
		  }

		  else if ( sArgv.compare( "-c" ) == 0 && argc >= ( i + 1 ) )
		  {
				dCols = atoi( argv[ i + 1 ] );
				cout << "Number Of Cols Set: " << dCols << endl;
		  }

		  else if ( sArgv.compare( "-d" ) == 0 && argc >= ( i + 1 ) )
		  {
				dDeintAlg = atoi( argv[ i + 1 ] );
				cout << "Deinterlace Set: " << dDeintAlg << endl;
		  }

		  //else if ( sArgv.compare( "-s" ) == 0 && argc >= ( i + 1 ) )
		  //{
		  //		nSkipperR = atoi( argv[ i + 1 ] );
		  //		cout << "Number of skipper repeat measurements set: " << nSkipperR << endl;
		  //}

		  else if ( sArgv.compare( "-h" ) == 0 )
		  {
				USAGE( argv[ 0 ] );
				exit( EXIT_FAILURE );
		  }
	}

	//
	// Create an instance of the ARC controller API
	//
	//auto_ptr<CArcDevice> pArcDev( new CArcPCIe );

	CArcDevice *pArcDev = new CArcPCIe;

	cout << endl;

	try
	{
		//
		// Find all ARC PCI/e device
		//
		cout << SetDots( "Finding devices" );
        CArcPCIe::FindDevices();
		cout << "done!" << endl;

		//
		// Open a driver/device connection
		//
		cout << SetDots( "Opening first device" );
		pArcDev->Open( 0, dRows * dCols * nSkipperR * sizeof( unsigned short ) );
		cout << "done! Image Buffer VA: 0x" << hex
			 << pArcDev->CommonBufferVA()
			 << dec << endl;

		//
		// Setup the controller
		//
		cout << SetDots( "Setting up controller" );

		/* Setup controller syntax: void SetupController( bool bReset, bool bTdl, bool bPower, int dRows, int dCols, const
char *pszTimFile, const char *pszUtilFile, const char *pszPciFile, const bool& bAbort );

Parameters: bReset True to reset the controller. Typically be set to true.
bTdl: True to test the data link between the host computer and the host device ( PCI, PCIe ), and the host device
and the camera controller. Typically set to true.
bPower True to power-on the camera controller. Typically set to true.
dRows Image row dimension ( in pixels )
dCols Image column dimension ( in pixels )
pszTimFile DSP timing board file ( .lod )
pszUtilFile DSP utility board file ( .lod ). Default = NULL
pszPciFile DSP PCI board file ( .lod ). Default = NULL
bAbort Reference variable to allow external program to exit this method. Default = false
*/


		pArcDev->SetupController( true,true,true,dRows,dCols,sTimFile.c_str() );
        cout << "done!" << endl;

        //Select Amplifier
        std::cout<<"Setting amplifier. \n";
        int SOS_response=pArcDev->Command( TIM_ID, SOS, AMP_L);
        if (SOS_response != DON)
            std::cout<<"Amplifier settings could not be applied. \n";
        else
            std::cout<<"Amplifier selected. \n";

	CCDRestoreBiasVoltages(&pArcDev);
        //CCD Erase and SetBias Voltages
        //Step 1 - Apply the reset on the V clocks
        std::cout<<"Flush the charges from the CCD using the erase procedure. \n";
        CCDChargeFlushReset(&pArcDev);
        sleep(2);
        //Turn relay OFF
        std::cout<<"Turn the relay switch OFF and wait 5 seconds. \n";
	SetDACValueBias(&pArcDev, 11, 0);
        sleep(5);
        //Turn relay ON
        std::cout<<"Turn the relay switch ON and wait 5 seconds. \n";
	SetDACValueBias(&pArcDev, 11, 4000);
        sleep(5);
        //Set the correct biases
        std::cout<<"Set the correct biases for CCD operation. \n";
        CCDRestoreClockVoltages(&pArcDev);


        std::cout<<"Erase and reset procedure complete. \n";

        std::cout<<"Now setting video offset. \n";
        pArcDev->Command( TIM_ID, SBN, 0,  2, VID, 0 );
        pArcDev->Command( TIM_ID, SBN, 0,  3, VID, 0 );
        std::cout<<"done. \n";


        //cout << SetDots( "Setting up number of skipper repeat measurements\n");
        //CArcPCIe::Command( TIM_ID, SSR, nSkipperR);
        //int dReply = 0;
        //dReply = pArcDev.get()->Command( 2, 0x00535352, nSkipperR);
        //if ( dReply == 0x00444F4E ) cout<<" done!";
        //else printf("Error setting skipper sequences: %X\n", dReply);
        //pArcDev.get()->SetSkipperRepeats(8);





		//
		// Expose
		//
		pArcDev->Expose( fExpTime, dRows, dCols, bAbort, &cExposeListener );

		//
		// Deinterlace the image
		//
//		cout << SetDots( "Deinterlacing image" );
//		CArcDeinterlace cDlacer;
//		cDlacer.RunAlg( pArcDev.get()->CommonBufferVA(),
//						dRows,
//						dCols*nSkipperR,
//						dDeintAlg );
//		cout << "done!" << endl;

		//
		// Save the image to FITS
		//
		cout << SetDots( "Writing FITS" );
		CArcFitsFile cFits( "Image.fit", dRows, dCols*nSkipperR );
		cFits.Write( pArcDev->CommonBufferVA() );
		cout << "done!" << endl;

		//
		// Close the device connection
		//
		cout << SetDots( "Closing device" );
		pArcDev->Close();
		cout << "done!" << endl;
	}
	catch ( std::runtime_error &e )
	{
		cout << "failed!" << endl;
		cerr << endl << e.what() << endl;

		if ( pArcDev->IsReadout() )
		{
			pArcDev->StopExposure();
		}

		pArcDev->Close();
	}
	catch ( ... )
	{
		cerr << endl << "Error: unknown exception occurred!!!" << endl;

		if ( pArcDev->IsReadout() )
		{
			pArcDev->StopExposure();
		}

		pArcDev->Close();
	}
}

// +--------------------------------------------------------------------------------
// |  SetDots
// +--------------------------------------------------------------------------------
// |  This function just prints dots (...) for the output.
// +--------------------------------------------------------------------------------
std::string SetDots( const char *cStr )
{
      std::string sStr( cStr );

      for ( int i=int( sStr.length() ); i<40; i++ )
	  {
            sStr.append( "." );
	  }

      return sStr;
}