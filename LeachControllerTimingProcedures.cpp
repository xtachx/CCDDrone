/* *********************************************************************
 * This file contains timing related procedures that are required for
 * operation of a CCD. These processes include things like clock widths,
 * integration times and wait times between clocks.
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


/*!
 * CalculateTiming calculates the hex code needed to generate the timing
 * for the waveform that is being processed. The timing is 8 bits with:
 * If the 8th bit is 1, then the timing is 320 ns * <number represented by 1-7th bit>
 * If the 8th bit is 0, then the timing is 40 ns * <number represented by 1-7th bit>
 *
 * @param time_in_us Timing of the waveform in micro-seconds.
 * @return Hex value needed to append the correct timing to a waveform
 */

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
        timing_bigmult = ( iTime_ns/320 ) | 0x80;
        timing_dsp = timing_bigmult;
    } else {
        //if between 640ns and 40ns, 640 is a closer match, then use that
        bigreminder=iTime_ns % 320 > 160 ? 320-iTime_ns % 320 : iTime_ns % 320;
        littlereminder= iTime_ns % 40 > 20 ? 40-iTime_ns % 40 : iTime_ns % 40 ;

        if (bigreminder <= littlereminder ) {

            timing_bigmult= (iTime_ns/320) | 0x80 ;
            timing_dsp = timing_bigmult;

        } else {

            timing_littlemult= iTime_ns/40 ;
            timing_dsp = timing_littlemult;

        }

    }

    timing_dsp = timing_dsp<<16;

    return timing_dsp;

}


/*!
 * ApplyNewIntegralTimeAndGain calculates and applies a new integral time. It also
 * selects an appropreate integrator (fast or slow) based on the new integral time
 * and applies that as well. The fast integrator is selected if the integration time
 * is < 4.5 micro-seconds, otherwise the slow integrator is picked.
 * The function also applies a specified gain on the Leach
 * dual-slope integrator.
 *
 * @param integralTime Integration time in micro-seconds.
 * @param gain Gain of the dual-slope integrator stage.
 * @return 0 if success and -1 otherwise.
 */


int LeachController::ApplyNewIntegralTimeAndGain(double integralTime, int gain)
{

    int timing_dsp = this->CalculateTiming(integralTime);

    /*Set gain and speed
     *SPEED = 0 for slow, 1 for fast*/
    if ( integralTime < 4.5 ) this->CCDParams.ItgSpeed = 1;
    else this->CCDParams.ItgSpeed = 0;

    this->ApplyGainAndSpeed(gain, this->CCDParams.ItgSpeed);

    /*Set integral time*/
    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, CIT, timing_dsp);
    if ( dReply == 0x00444F4E ) {
        return 0;
    } else {
        printf("Error setting the integration time: %X\n", dReply);
        return -1;
    }

}


/*!
 * ApplyGainAndSpeed works in tandem with ApplyNewIntegralTimeAndGain.
 * This function applies a specified dual slope integrator and integrator speed.
 * This function should only be called by ApplyNewIntegralTimeAndGain.
 * @param gain Dual slope integrator gain.
 * @param speed Integrator speed.
 * @return 0 for success and -1 otherwise.
 */

int LeachController::ApplyGainAndSpeed( int gain, int speed) {

    /*Command syntax is  SGN  #GAIN  #SPEED.
     * Note: Gain can be one of = 1, 2, 5 or 10
     * #SPEED = 0 for slow, 1 for fast */

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, SGN, gain, speed);
    if ( dReply == 0x00444F4E ) {
        return 0;
    } else {
        printf("Error setting the speed and gain: %X\n", dReply);
        return -1;
    }
}


/*!
 * ApplyNewPedestalIntegralWait applies a wait time before the pedestal of the CDS
 * signal is integrated. This is needed for the signal to settle after the clocks
 * are exercised.
 *
 * @param pedestalWaitTime pre-Pedestal integration wait time in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */

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


/*!
 * ApplyNewSignalIntegralWait applies a wait time before the signal of the CDS
 * is integrated. This is needed for the signal to settle after the SW is
 * exercised
 * @param signalWaitTime pre-Signal integration wait time in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */
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

/*!
 * ApplyDGWidth controls the Dump Gate width.
 * @param newDGWidth Dump Gate width in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */

int LeachController::ApplyDGWidth(double newDGWidth) {

    int timing_dsp = this->CalculateTiming(newDGWidth);

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, DGW, timing_dsp);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the DG width / time: %X\n", dReply);
        return -1;
    }

}

/*!
 * ApplyOGWidth may be used to change the Output Gate width.
 * @param newOGWidth Output Gate width in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */
int LeachController::ApplyOGWidth(double newOGWidth) {

    int timing_dsp = this->CalculateTiming(newOGWidth);

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, OGW, timing_dsp);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the OG width / time: %X\n", dReply);
        return -1;
    }

}

/*!
 * ApplySkippingRGWidth may be used to change the Reset Gate width.
 * @param newRGWidth Reset Gate width in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */
int LeachController::ApplySkippingRGWidth(double newRGWidth) {

    int timing_dsp = this->CalculateTiming(newRGWidth);

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, RSW, timing_dsp);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the RG width / time: %X\n", dReply);
        return -1;
    }

}


/*!
 * ApplySummingWellWidth may be used to change the width of the Summing Well.
 * @param newSWWidth Summing well width in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */
int LeachController::ApplySummingWellWidth(double newSWWidth) {

    int timing_dsp = this->CalculateTiming(newSWWidth);

    int dReply = 0;
    dReply = pArcDev->Command( TIM_ID, SWW, timing_dsp);
    if ( dReply == DON ) {
        return 0;
    } else {
        printf("Error setting the SW width / time: %X\n", dReply);
        return -1;
    }

}

/*!
 * ApplyVClockWidths may be used to change the width of the V-clocks.
 * @param newVClock Summing well width in micro-seconds.
 * @param newVOverlap Summing well width in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */
int LeachController::ApplyVClockWidths(double newVClock, double newVOverlap) {


    int dReplyVck = 0;
    int dReplyVov = 0;

    int timing_dsp_vck = this->CalculateTiming(newVClock);
    int timing_dsp_ov = this->CalculateTiming(newVOverlap);


    dReplyVck = pArcDev->Command( TIM_ID, CPP, timing_dsp_vck);
    dReplyVov = pArcDev->Command( TIM_ID, CPL, timing_dsp_ov);


    if ( dReplyVck == DON && dReplyVov == DON ) {
        return 0;
    } else {
        if (dReplyVck != DON) printf("Error setting the V clock width / time: %X\n", dReplyVck) ;
        if (dReplyVov != DON) printf("Error setting the V overlap width / time: %X\n", dReplyVov) ;
        return -1;
    }

}


/*!
 * ApplyHClockWidths may be used to change the width of the H-clocks.
 * @param newHClock Summing well width in micro-seconds.
 * @param newHOverlap Summing well width in micro-seconds.
 * @return 0 for success and -1 otherwise.
 */
int LeachController::ApplyHClockWidths(double newHClock, double newHOverlap) {

    int dReplyHck = 0;
    int dReplyHov = 0;

    int timing_dsp_hck = this->CalculateTiming(newHClock);
    int timing_dsp_hov = this->CalculateTiming(newHOverlap);


    dReplyHck = pArcDev->Command( TIM_ID, CSS, timing_dsp_hck);
    dReplyHov = pArcDev->Command( TIM_ID, CSL, timing_dsp_hov);

    if ( dReplyHck == DON && dReplyHov == DON) {
        return 0;
    } else {
        if (dReplyHck != DON) printf("Error setting the H clock width / time: %X\n", dReplyHck);
        if (dReplyHov != DON) printf("Error setting the H overlap width / time: %X\n", dReplyHov);
        return -1;
    }

}
