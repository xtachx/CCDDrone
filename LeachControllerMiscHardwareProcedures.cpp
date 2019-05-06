/* *********************************************************************
 * This file contains miscellaneous procedures that are required for
 * operation of a CCD. These processes include things like the Charge Erase,
 * CCD Purge, set the clock directions and ccd startup.
 *
 * Created by Pitam Mitra on 2019-02-13.
 * *********************************************************************
 */

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CExpIFace.h"
#include "ArcDefs.h"


#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"


/*
 * Apply +9V to all H-clocks in effect making the
 * pixel array have a uniform electric potential at the top.
 * Used for: Charge erase procedure.
 */

void LeachController::ApplyAllPositiveVPixelArray()
{

    this->SetDACValueClock(0, 9.0, 9.0); //Channel 0: Min 9V Max 9V
    this->SetDACValueClock(1, 9.0, 9.0); //Channel 1: Min 9V Max 9V
    this->SetDACValueClock(2, 9.0, 9.0); //Channel 2: Min 9V Max 9V

}


/*
 * Restore the clock voltages after the charge erase procedure.
 */

void LeachController::RestoreVClockVoltages(void )
{

    this->SetDACValueClock(0, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 0: V1
    this->SetDACValueClock(1, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 1: V2
    this->SetDACValueClock(2, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 2: V3

}



/*
 * Procedure to startup the leach controller and set image size.
 * Prepare for IDLE clocking.
 */
void LeachController::StartupController(void )
{

    //RESET
    pArcDev->ResetController();
    //Test Data Link
    for (int i=0; i<123; i++) {
        if ( pArcDev->Command( TIM_ID, TDL, 0x123456 ) != 0x123456 ) {
            std::cout<<"TIM TDL failed.\n";
            throw 10;
        }
    }

    //Load controller file
    pArcDev->LoadControllerFile(this->CCDParams.sTimFile.c_str());
    pArcDev->Command( TIM_ID, PON ); //Power ON
    pArcDev->SetImageSize(this->CCDParams.dRows,this->CCDParams.dCols); //Set image size for idle

}


/*
 * Charge erase procedure sequence. Please refer to the berkeley manual
 * for an explanation of how this works.
 */
void LeachController::PerformEraseProcedure(void)
{

    std::cout<<"Setting pixel array to (9V,9V)\n";
    this->ApplyAllPositiveVPixelArray();

    std::cout<<"Switching Vsub / relay OFF (pin 11) and wait 5 seconds.\n";
    this->CCDBiasToggle(0);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout<<"Switch Vsub / relay ON (pin 11). After 5 seconds, the clock voltages will be restored.\n";
    this->CCDBiasToggle(1);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    this->RestoreVClockVoltages();
    std::cout<<"Clock voltages restored. Erase procedure is now complete.\n";

}


/*
 * Super sequencer only
 */
int LeachController::SetSSR(void )
{

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, SSR, this->CCDParams.nSkipperR);
    if ( dReply == 0x00444F4E ) {
        return 0;
    } else {
        printf("Error setting skipper sequences: %X\n", dReply);
        return -1;
    }


}


void LeachController::ApplyNewSequencer(std::string seqFile)
{
    pArcDev->LoadControllerFile(seqFile.c_str());
}

int LeachController::SetCCDType(void )
{

    int _iCCDType;
    if (this->CCDParams.CCDType=="DES")
        _iCCDType=1;
    else if (this->CCDParams.CCDType=="SK")
        _iCCDType=0;
    else {
        std::cout<<"CCD Type is unknown. Could not set the sequencer's CCD type.";
        return -2;
    }
    int dReply = 0;

    dReply = pArcDev->Command( TIM_ID,SAT,_iCCDType);
    if ( dReply == 0x00444F4E ) {
        std::cout<<"CCD Type has been set in the sequencer.\n";
        return 0;
    } else {
        printf("Error setting sequencer CCD type: %X\n", dReply);
        return -1;
    }


}

int LeachController::SetVDR(void )
{

    int dReply = 0;
    int _snd_VDRxn = 0;

    if (this->CCDParams.VClkDirection=="1")
        _snd_VDRxn = 0;
    else if(this->CCDParams.VClkDirection=="2")
        _snd_VDRxn = 1;
    else {
        std::cout<<"V-Clock direction is invalid. Setting the normal clock to move charges towards side 1.\n";
        _snd_VDRxn = 0;
    }

    dReply = pArcDev->Command( TIM_ID, VDR, _snd_VDRxn);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting v-clock sequences: %X\n", dReply);
        return -1;
    }

}

/*SetHDR must run AFTER SBN. This is to keep backwards compatibility
 *with OWL*/
int LeachController::SetHDR(void )
{

    int dReply = 0;
    int HDR_response;

    if (this->CCDParams.HClkDirection == "UL") {
        HDR_response = pArcDev->Command(TIM_ID, HDR, AMP_LR);
    } else if (this->CCDParams.HClkDirection == "U") {
        HDR_response = pArcDev->Command(TIM_ID, HDR, AMP_L);
    } else if (this->CCDParams.HClkDirection == "L") {
        HDR_response = pArcDev->Command(TIM_ID, HDR, AMP_R);

    } else {
        std::cout << "The Serial Register / H-clock direction selected does not exist. Stop and verify!\n";
        return -1;
    }

    if (HDR_response != DON)
        std::cout << "Serial Register / H-clock settings could not be applied. \n";
    else
        std::cout << "Serial Register / H-clock direction selected. \n";

    return 0;
}

int LeachController::SelectAmplifierAndHClocks(void )
{

    int SOS_response;
    if (this->CCDParams.AmplifierDirection == "UL") {
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_LR);

    } else if (this->CCDParams.AmplifierDirection == "U") {
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_L);

    } else if (this->CCDParams.AmplifierDirection == "L") {
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_R);

    } else {
        std::cout << "The amplifier selected does not exist. Stop and verify!\n";
        return -1;
    }

    if (SOS_response != DON) {
        std::cout << "Amplifier settings could not be applied. \n";
        return -2;
    } else {
        std::cout << "Amplifier selected. \n";
        return 0;
    }


}

int LeachController::CalculateTiming(double time_in_us) {

    //First, convert number to ns. The ($var+0.5)/1 converts float to an int

    if (time_in_us>163){
        std::cout<<"The range for integration time is 40ns to 163 usec.\n"<<
                 "The value entered is out of bounds, so restricting the time to 163 usec.\n";

        time_in_us = 163;
    }

    double fiTime_ns = time_in_us*1000;
    int iTime_ns = (int) fiTime_ns;

    int timing_bigmult, bigreminder, littlereminder,timing_littlemult;
    int timing_dsp;

    if (iTime_ns > 4000) {
        timing_bigmult = ( iTime_ns/640 ) | 0x80;
        timing_dsp = timing_bigmult;
    } else {
        //if between 640ns and 40ns, 640 is a closer match, then use that
        bigreminder=iTime_ns % 640 > 320 ? 640-iTime_ns % 640 : iTime_ns % 640;
        littlereminder= iTime_ns % 40 > 20 ? 40-iTime_ns % 40 : iTime_ns % 40 ;

        if (bigreminder <= littlereminder ) {

            timing_bigmult= (iTime_ns/640) | 0x80 ;
            timing_dsp = timing_bigmult;

        } else {

            timing_littlemult= iTime_ns/40 ;
            timing_dsp = timing_littlemult;

        }

    }

    timing_dsp = timing_dsp<<16;

    return timing_dsp;

}

int LeachController::ApplyNewIntegralTime(double integralTime)
{

    int timing_dsp = this->CalculateTiming(integralTime);

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, CIT, timing_dsp);
    if ( dReply == 0x00444F4E ) {
        return 0;
    } else {
        printf("Error setting the integration time: %X\n", dReply);
        return -1;
    }

}

int LeachController::ApplyNewPedestalIntegralWait(double pedestalWaitTime){

    int timing_dsp = this->CalculateTiming(pedestalWaitTime);

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, CPR, timing_dsp);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the pedestal wait time: %X\n", dReply);
        return -1;
    }

}

int LeachController::ApplyNewSignalIntegralWait(double signalWaitTime){

    int timing_dsp = this->CalculateTiming(signalWaitTime);

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, CPO, timing_dsp);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the pedestal wait time: %X\n", dReply);
        return -1;
    }

}


int LeachController::ApplyPBIN(int NPBIN){

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, NPB, NPBIN);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the parallel binning: %X\n", dReply);
        return -1;
    }

}

int LeachController::ApplySBIN(int NSBIN){

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, NSB, NSBIN);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the serial binning: %X\n", dReply);
        return -1;
    }

}