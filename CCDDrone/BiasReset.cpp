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

    int clkVoltADC;
    double ADCVal = 4095.0*(PD-MIN_CLOCK)/(MAX_CLOCK-MIN_CLOCK);
    clkVoltADC = (int)ADCVal;
    return 0x00000FFF & clkVoltADC;
}

void SetDACValueClock(arc::device::CArcDevice** pArcDev, int dac_chan, double dmin, double dmax){

    arc::device::CArcDevice *pArcDevice = *pArcDev;

    int resp1, resp2;

    resp1 = pArcDevice->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*dac_chan, CLK, ClockVoltToADC(dmax) ); //MAX
    resp2 = pArcDevice->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*dac_chan+1, CLK, ClockVoltToADC(dmin) ); //MIN

    if (resp1 != 0x00444F4E || resp2 != 0x00444F4E )
        printf ("Error setting CVIon channel: %d | code (max, min): (%X, %X)\n", dac_chan, resp1, resp2);

    //printf ("Written: %X, %X, %d, %d , %X, %d\n", TIM_ID, SBN, CLOCK_JUMPER, 2*DAC, CLK, ClockVoltToADC(dmax));
}


void SetDACValueBias(arc::device::CArcDevice** pArcDev, int dac_chan, int val){

    arc::device::CArcDevice *pArcDevice = *pArcDev;

    int resp;

    resp = pArcDevice->Command( TIM_ID, SBN, CLOCK_JUMPER, dac_chan, VID, val ); //MAX

    if (resp != 0x00444F4E )
        printf ("Error setting CVIon channel: %d | code: %X\n", dac_chan, resp);

    //printf ("Written: %X, %X, %d, %d , %X, %d\n", TIM_ID, SBN, CLOCK_JUMPER, dac_chan, VID, val);

}



void CCDChargeFlushReset(arc::device::CArcDevice** pArcDev){

    SetDACValueClock(pArcDev, 0, 9.0, 9.0); //Channel 0: Min 9V Max 9V
    SetDACValueClock(pArcDev, 1, 9.0, 9.0); //Channel 1: Min 9V Max 9V
    SetDACValueClock(pArcDev, 2, 9.0, 9.0); //Channel 2: Min 9V Max 9V

}


void CCDRestoreClockVoltages(arc::device::CArcDevice** pArcDev){

    /*Set Clocks*/
    std::cout<<"Setting clock voltages\n";
    SetDACValueClock(pArcDev, 0, -1.0, 6.0); //Channel 0: V1
    SetDACValueClock(pArcDev, 1, -1.0, 6.0); //Channel 1: V2
    SetDACValueClock(pArcDev, 2, -1.0, 6.0); //Channel 2: V3

    SetDACValueClock(pArcDev, 6, 0.5, 5.0); //Channel 6: TG

    SetDACValueClock(pArcDev, 12, 0.0, 5.0); //Channel 12: H1L
    SetDACValueClock(pArcDev, 13, 0.0, 5.0); //Channel 13: H2L
    SetDACValueClock(pArcDev, 14, 0.0, 5.0); //Channel 14: H3L
    SetDACValueClock(pArcDev, 15, 0.0, 5.0); //Channel 15: H1U
    SetDACValueClock(pArcDev, 16, 0.0, 5.0); //Channel 16: H2U
    SetDACValueClock(pArcDev, 17, 0.0, 5.0); //Channel 17: H3U

    SetDACValueClock(pArcDev, 18, 0.0, 5.0); //Channel 18: SWL
    SetDACValueClock(pArcDev, 20, 0.0, -4.0); //Channel 20: RGL

    SetDACValueClock(pArcDev, 21, 0.0, -4.0); //Channel 21: RGU
    SetDACValueClock(pArcDev, 23, 0.0, 5.0); //Channel 23:


}



void CCDRestoreBiasVoltages(arc::device::CArcDevice** pArcDev){

    /*Set Biases*/
    std::cout<<"Setting bias voltages\n";
    SetDACValueBias(pArcDev, 0,3604);
    SetDACValueBias(pArcDev, 1,3604);
    SetDACValueBias(pArcDev, 2,0);
    SetDACValueBias(pArcDev, 3,0);


    //VR(1-4)
    SetDACValueBias(pArcDev, 4,2050);
    SetDACValueBias(pArcDev, 5,2050);
    SetDACValueBias(pArcDev, 6,0);
    SetDACValueBias(pArcDev, 7,0);

    //OG(1-4)
    SetDACValueBias(pArcDev, 8,1810);
    SetDACValueBias(pArcDev, 9,1810);
    SetDACValueBias(pArcDev, 10,0);
    SetDACValueBias(pArcDev, 11,0);


    //VSUB
    SetDACValueBias(pArcDev, 12,0);
    SetDACValueBias(pArcDev, 13,0);


}


