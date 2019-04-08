//
// Created by Pitam Mitra on 2019-02-13.
//

#include <string>
#include <iostream>
#include <fstream>

#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CExpIFace.h"
#include "ArcDefs.h"


#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"


/*Click driver jumper setting*/
#define CLOCK_JUMPER 2

/* Clock driver max volts +10 and min volts -10*/
#define MAX_CLOCK 13.0
#define MIN_CLOCK -13.0




LeachController::LeachController(std::string INIFileLoc)
{

    /*New ArcDevice*/
    pArcDev = new arc::device::CArcPCIe;

    /*Open the Arc controller by loading a timing file*/
    arc::device::CArcPCIe::FindDevices();
    pArcDev->Open(0);

    this->INIFileLoc = INIFileLoc;


}

LeachController::~LeachController()
{

    pArcDev->Close();

}


void LeachController::ApplyAllCCDBasic(void ){

    if (this->CCDParams.super_sequencer){
        /*Set CCD type for the sequencer - DES or SK sequence*/
        this->SetCCDType();
        /*Now set the amplifier direction since this affects IDLE mode*/
        this->SelectAmplifierAndHClocks();
        /*AFTER amplifiers are selected, change the H-clock directions if necessary*/
        if (this->CCDParams.AmplifierDirection != this->CCDParams.HClkDirection)
            this->SetHDR();
        /*Set V-Clock direction*/
        this->SetVDR();
        /*Next, set the integral time*/
        this->ApplyNewIntegralTime(this->CCDParams.IntegralTime);
    }

    if (this->CCDParams.CCDType == "SK"){
        this->SetSSR();
    }

}



/*
 *The lines are UW specific, so keeping this function here makes sense since people might edit
 *these values if they change the second stage board
 */

void LeachController::ApplyAllCCDClocks(void )
{
    /*Set Clocks*/
    this->SetDACValueClock(0, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 0: V1
    this->SetDACValueClock(1, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 1: V2
    this->SetDACValueClock(2, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 2: V3

    this->SetDACValueClock(6, this->ClockParams.tg_lo,this->ClockParams.tg_hi); //Channel 6: TG

    this->SetDACValueClock(12, this->ClockParams.l_hclock_lo, this->ClockParams.l_hclock_hi); //Channel 12: H1L
    this->SetDACValueClock(13, this->ClockParams.l_hclock_lo, this->ClockParams.l_hclock_hi); //Channel 13: H2L
    this->SetDACValueClock(14, this->ClockParams.l_hclock_lo, this->ClockParams.l_hclock_hi); //Channel 14: H3L
    this->SetDACValueClock(15, this->ClockParams.u_hclock_lo, this->ClockParams.u_hclock_hi); //Channel 15: H1U
    this->SetDACValueClock(16, this->ClockParams.u_hclock_lo, this->ClockParams.u_hclock_hi); //Channel 16: H2U
    this->SetDACValueClock(17, this->ClockParams.u_hclock_lo, this->ClockParams.u_hclock_hi); //Channel 17: H3U

    this->SetDACValueClock(18, this->ClockParams.sw_lo, this->ClockParams.sw_hi); //Channel 18: SWL
    this->SetDACValueClock(23, this->ClockParams.sw_lo, this->ClockParams.sw_hi); //Channel 23: SWU

    //Reset gate needs to be checked against the current timing file and be flipped if necessary
    if (this->CCDParams.InvRG)
    {
        this->SetDACValueClock(20, this->ClockParams.rg_hi, this->ClockParams.rg_lo); //Channel 20: RGL
        this->SetDACValueClock(21, this->ClockParams.rg_hi, this->ClockParams.rg_lo); //Channel 21: RGU
    }
    else
    {
        this->SetDACValueClock(20, this->ClockParams.rg_lo, this->ClockParams.rg_hi); //Channel 20: RGL
        this->SetDACValueClock(21, this->ClockParams.rg_lo, this->ClockParams.rg_hi); //Channel 21: RGU
    }

}


void LeachController::ApplyAllBiasVoltages(void )
{

    /*Set Biases*/
    //Vdd
    this->SetDACValueBias(0,BiasVoltToADC(this->BiasParams.vdd,0));
    this->SetDACValueBias(1,BiasVoltToADC(this->BiasParams.vdd,1));
    this->SetDACValueBias(2,0);
    this->SetDACValueBias(3,0);


    //VR(1-4)
    if (this->CCDParams.CCDType=="DES")
    {
        this->SetDACValueBias(4,BiasVoltToADC(this->BiasParams.vref,4));
        this->SetDACValueBias(5,BiasVoltToADC(this->BiasParams.vref,5));
    }
    else
    {
        this->SetDACValueBias(4,BiasVoltToADC(this->BiasParams.vrefsk,4));
        this->SetDACValueBias(5,BiasVoltToADC(this->BiasParams.vrefsk,5));
    }
    //DrainL and DrainU
    if(this->CCDParams.CCDType=="SK")
    {
        this->SetDACValueBias(6,BiasVoltToADC(this->BiasParams.drain,6));
        this->SetDACValueBias(7,BiasVoltToADC(this->BiasParams.drain,7));
    }

    //OG(1-4)
    if (this->CCDParams.CCDType=="DES")
    {
        this->SetDACValueBias(8,BiasVoltToADC(this->BiasParams.opg,8));
        this->SetDACValueBias(9,BiasVoltToADC(this->BiasParams.opg,9));
    }

    this->SetDACValueBias(10,0);
    //Controls Relay for battery box
    this->SetDACValueBias(11,BiasVoltToADC(this->BiasParams.battrelay,11));

    //VSUB
    //this->SetDACValueBias(pArcDev, 12,0);
    //this->SetDACValueBias(pArcDev, 13,0);

    //Video Offsets, channels 2 and 3 on the video board. 3 is R and 2 is L
    this->SetDACValueVideoOffset(2, this->BiasParams.video_offsets_U);
    this->SetDACValueVideoOffset(3, this->BiasParams.video_offsets_L);

}

