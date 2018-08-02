#include <iostream>
#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CArcFitsFile.h"
#include "CExpIFace.h"
#include "ArcDefs.h"


/* ******************************************************************************
 * Code to perform the well known berkeley CCD charge flush reset procedure!
 * ******************************************************************************/


/*Click driver jumper setting*/
#define CLOCK_JUMPER 2

/* Clock driver max volts +10 and min volts -10*/
#define MAX_CLOCK 10.0
#define MIN_CLOCK -10.0


int ClockVoltToADC(double PD){
    double ADCVal = 4095.0*(PD-MIN_CLOCK)/(MAX_CLOCK-MIN_CLOCK);
    return (int)ADCVal;
}

void SetDACValue(arc::device::CArcDevice** pArcDev, int DAC, double dmin, double dmax){

    arc::device::CArcDevice *pArcDevice = *pArcDev;

    int resp1, resp2;

    resp1 = pArcDevice->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*DAC, CLK, ClockVoltToADC(dmax) ); //MAX
    resp2 = pArcDevice->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*DAC+1, CLK, ClockVoltToADC(dmin) ); //MIN

    if (resp1 != 0x00444F4E || resp2 != 0x00444F4E )
        printf ("Error setting CVIon channel: %d | code (max, min): (%X, %X)\n", DAC, resp1, resp2);


}

void CCDChargeFlushReset(arc::device::CArcDevice** pArcDev){

    SetDACValue(pArcDev, 0, 9.0, 9.0); //Channel 0: Min 9V Max 9V
    SetDACValue(pArcDev, 1, 9.0, 9.0); //Channel 1: Min 9V Max 9V
    SetDACValue(pArcDev, 2, 9.0, 9.0); //Channel 2: Min 9V Max 9V

}


void CCDRestoreBiasVoltages(arc::device::CArcDevice** pArcDev){

    SetDACValue(pArcDev, 0, -1.0, 6.0); //Channel 0: V1
    SetDACValue(pArcDev, 1, -1.0, 6.0); //Channel 1: V2
    SetDACValue(pArcDev, 2, -1.0, 6.0); //Channel 2: V3

    SetDACValue(pArcDev, 6, -1.0, 6.0); //Channel 6: TG

    SetDACValue(pArcDev, 12, 0.0, 5.0); //Channel 12: H1L
    SetDACValue(pArcDev, 13, 0.0, 5.0); //Channel 13: H2L
    SetDACValue(pArcDev, 14, 0.0, 5.0); //Channel 14: H3L
    SetDACValue(pArcDev, 15, 0.0, 5.0); //Channel 15: H1U
    SetDACValue(pArcDev, 16, 0.0, 5.0); //Channel 16: H2U
    SetDACValue(pArcDev, 17, 0.0, 5.0); //Channel 17: H3U

    SetDACValue(pArcDev, 18, 0.0, 5.0); //Channel 18: SWL
    SetDACValue(pArcDev, 20, 0.0, -4.0); //Channel 20: RGL

    SetDACValue(pArcDev, 21, 0.0, -4.0); //Channel 21: RGU
    SetDACValue(pArcDev, 23, 0.0, 5.0); //Channel 23: SWU


}




