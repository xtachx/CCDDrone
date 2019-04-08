//
// Created by Pitam Mitra on 2019-02-13.
//

#include <string>
#include <iostream>
#include <fstream>
#include <cmath>

#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CExpIFace.h"
#include "ArcDefs.h"

#include "picosha2.h"

#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"
#include "INIReader.h"


/*Click driver jumper setting*/
#define CLOCK_JUMPER 2
#define VIDEO_JUMPER 0

/* Clock driver max volts +10 and min volts -10*/
#define MAX_CLOCK 13.0
#define MIN_CLOCK -13.0



int LeachController::ClockVoltToADC(double PD)
{

    int clkVoltADC;
    double ADCVal = 4095.0*(PD-MIN_CLOCK)/(MAX_CLOCK-MIN_CLOCK);
    clkVoltADC = (int)ADCVal;
    return 0x00000FFF & clkVoltADC;
}

int LeachController::BiasVoltToADC(double PD, int line)
{

    int biasVoltADC;
    double ADCVal;
    PD = fabs(PD);

    //Line 0-7 have 25V reference, lines 8-12 have a 5V reference **CHECK**!!
    if (line<8)
        ADCVal = PD*4095/25.0;
    else
        ADCVal = PD*4095/5.0;

    biasVoltADC = (int)ADCVal;
    if (biasVoltADC > 4095)
        printf ("Warning: Bias on line %d is %d which is more than the limit 4095.\n ",line,biasVoltADC);

    return 0x00000FFF & biasVoltADC;

}

void LeachController::SetDACValueClock(int dac_chan, double dmin, double dmax)
{

    int resp1, resp2;

    resp1 = this->pArcDev->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*dac_chan, CLK, ClockVoltToADC(dmax) ); //MAX
    resp2 = this->pArcDev->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*dac_chan+1, CLK, ClockVoltToADC(dmin) ); //MIN

    if (resp1 != 0x00444F4E || resp2 != 0x00444F4E )
        printf ("Error setting CVIon channel: %d | code (max, min): (%X, %X)\n", dac_chan, resp1, resp2);

}


void LeachController::SetDACValueBias(int dac_chan, int val)
{

    int resp;

    resp = this->pArcDev->Command( TIM_ID, SBN, CLOCK_JUMPER, dac_chan, VID, val ); //MAX

    if (resp != 0x00444F4E )
        printf ("Error setting CVIon channel: %d | code: %X\n", dac_chan, resp);


}


// SBN 0 2,3 VID 0-4095
void LeachController::SetDACValueVideoOffset(int dac_chan, int val)
{

    /*Validate inputs*/
    if (dac_chan != 2 && dac_chan != 3){
        printf ("Incorrect video channel. Please check the video output channel.\n");
        return;
    }

    if (val < 0 || val > 4095 ){
        printf("Video offset value must be between 0 - 16383.\n");
        //return;
    }

    int resp;

    resp = this->pArcDev->Command( TIM_ID, SBN, VIDEO_JUMPER, dac_chan, VID, val );

    if (resp != 0x00444F4E )
        printf ("Error setting video offset on channel: %d | code: %X\n", dac_chan, resp);


}


void LeachController::CCDBiasToggle (bool state)
{

    if (state)
        this->SetDACValueBias(11,BiasVoltToADC(this->BiasParams.battrelay,11));
    else
        this->SetDACValueBias(11,0);

}

void LeachController::IdleClockToggle (void )
{

    this->pArcDev->Command(TIM_ID, IDL);

}

