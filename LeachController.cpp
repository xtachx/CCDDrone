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


/*
 *The lines are UW specific, so keeping this function here makes sense since people might edit
 *these values if they change the second stage board
 */

void LeachController::ApplyAllCCDClocks(CCDVariables &_CCDSettings, ClockVariables &_clockSettings)
{
    /*Set Clocks*/
    this->SetDACValueClock(0, _clockSettings.vclock_lo, _clockSettings.vclock_hi); //Channel 0: V1
    this->SetDACValueClock(1, _clockSettings.vclock_lo, _clockSettings.vclock_hi); //Channel 1: V2
    this->SetDACValueClock(2, _clockSettings.vclock_lo, _clockSettings.vclock_hi); //Channel 2: V3

    this->SetDACValueClock(6, _clockSettings.tg_lo,_clockSettings.tg_hi); //Channel 6: TG

    this->SetDACValueClock(12, _clockSettings.l_hclock_lo, _clockSettings.l_hclock_hi); //Channel 12: H1L
    this->SetDACValueClock(13, _clockSettings.l_hclock_lo, _clockSettings.l_hclock_hi); //Channel 13: H2L
    this->SetDACValueClock(14, _clockSettings.l_hclock_lo, _clockSettings.l_hclock_hi); //Channel 14: H3L
    this->SetDACValueClock(15, _clockSettings.u_hclock_lo, _clockSettings.u_hclock_hi); //Channel 15: H1U
    this->SetDACValueClock(16, _clockSettings.u_hclock_lo, _clockSettings.u_hclock_hi); //Channel 16: H2U
    this->SetDACValueClock(17, _clockSettings.u_hclock_lo, _clockSettings.u_hclock_hi); //Channel 17: H3U

    this->SetDACValueClock(18, _clockSettings.sw_lo, _clockSettings.sw_hi); //Channel 18: SWL
    this->SetDACValueClock(23, _clockSettings.sw_lo, _clockSettings.sw_hi); //Channel 23: SWU

    //Reset gate needs to be checked against the current timing file and be flipped if necessary
    if (_CCDSettings.InvRG)
    {
        this->SetDACValueClock(20, _clockSettings.rg_hi, _clockSettings.rg_lo); //Channel 20: RGL
        this->SetDACValueClock(21, _clockSettings.rg_hi, _clockSettings.rg_lo); //Channel 21: RGU
    }
    else
    {
        this->SetDACValueClock(20, _clockSettings.rg_lo, _clockSettings.rg_hi); //Channel 20: RGL
        this->SetDACValueClock(21, _clockSettings.rg_lo, _clockSettings.rg_hi); //Channel 21: RGU
    }

}


void LeachController::ApplyAllBiasVoltages(CCDVariables &_CCDSettings, BiasVariables &_BiasSettings )
{

    /*Set Biases*/
    //Vdd
    this->SetDACValueBias(0,BiasVoltToADC(_BiasSettings.vdd,0));
    this->SetDACValueBias(1,BiasVoltToADC(_BiasSettings.vdd,1));
    this->SetDACValueBias(2,0);
    this->SetDACValueBias(3,0);


    //VR(1-4)
    if (_CCDSettings.CCDType=="DES")
    {
        this->SetDACValueBias(4,BiasVoltToADC(_BiasSettings.vref,4));
        this->SetDACValueBias(5,BiasVoltToADC(_BiasSettings.vref,5));
    }
    else
    {
        this->SetDACValueBias(4,BiasVoltToADC(_BiasSettings.vrefsk,4));
        this->SetDACValueBias(5,BiasVoltToADC(_BiasSettings.vrefsk,5));
    }
    //DrainL and DrainU
    if(_CCDSettings.CCDType=="SK")
    {
        this->SetDACValueBias(6,BiasVoltToADC(_BiasSettings.drain,6));
        this->SetDACValueBias(7,BiasVoltToADC(_BiasSettings.drain,7));
    }

    //OG(1-4)
    if (_CCDSettings.CCDType=="DES")
    {
        this->SetDACValueBias(8,BiasVoltToADC(_BiasSettings.opg,8));
        this->SetDACValueBias(9,BiasVoltToADC(_BiasSettings.opg,9));
    }

    this->SetDACValueBias(10,0);
    //Controls Relay for battery box
    this->SetDACValueBias(11,BiasVoltToADC(_BiasSettings.battrelay,11));

    //VSUB
    //this->SetDACValueBias(pArcDev, 12,0);
    //this->SetDACValueBias(pArcDev, 13,0);

}

