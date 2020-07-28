/* *********************************************************************
 * This file contains functions that set the basic parameters,
 * clocks and bias lines of the CCD. Vdd toggling functions are also
 * implemented here.
 *
 * Created by Pitam Mitra on 2019-02-13.
 * *********************************************************************
 */

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



LeachController::LeachController(std::string INIFileLoc)
{

    /*New ArcDevice*/
    pArcDev = new arc::device::CArcPCIe;

    /*Open the Arc controller by loading a timing file*/
    arc::device::CArcPCIe::FindDevices();
    pArcDev->Open(0);

    this->INIFileLoc = INIFileLoc;

    /*Check if a controller is connected*/
    if ( !pArcDev->IsControllerConnected() )
        std::cout<<"Warning: A controller is not connected. Check if the unit is connected and powered on.\n";



}

LeachController::~LeachController()
{

    pArcDev->Close();

}

/* Function to apply all the basic CCD params.*/

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
        this->ApplyNewIntegralTimeAndGain(this->CCDParams.IntegralTime, this->CCDParams.Gain);
        /*Next, we need to set the Pedestal and Signal wait times before an integration begins*/
        this->ApplyNewPedestalIntegralWait(this->CCDParams.PedestalIntgWait);
        this->ApplyNewSignalIntegralWait(this->CCDParams.SignalIntgWait);
        /*Binning*/
        this->ApplyPBIN(this->CCDParams.ParallelBin);
        this->ApplySBIN(this->CCDParams.SerialBin);
        /*Timing of DG, OG and Reset*/
        this->ApplyDGWidth(this->CCDParams.DGWidth);
        this->ApplyOGWidth(this->CCDParams.OGWidth);
        this->ApplySkippingRGWidth(this->CCDParams.SKRSTWidth);
        this->ApplySummingWellWidth(this->CCDParams.SWWidth);
        this->ApplyHClockWidths(this->CCDParams.HWidth,this->CCDParams.HOverlapWidth);
        this->ApplyVClockWidths(this->CCDParams.VWidth,this->CCDParams.VOverlapWidth);

    }

    if (this->CCDParams.CCDType == "SK"){
        this->SetSSR();
    }

}



/*
 * Function to set all the clock voltages of the CCD.
 * The lines are UW specific (or rather specific to the US second stage board)
 * so keeping this function here makes sense since people might edit
 * these values if they change the second stage board.
 */

void LeachController::ApplyAllCCDClocks(void )
{
    /*Set Clocks*/

    /*These sets of clocks are common and did not change between Second stage generations.*/

    this->SetDACValueClock(0, this->ClockParams.one_vclock_lo, this->ClockParams.one_vclock_hi); //Channel 0: V1
    this->SetDACValueClock(1, this->ClockParams.one_vclock_lo, this->ClockParams.one_vclock_hi); //Channel 1: V2
    this->SetDACValueClock(2, this->ClockParams.one_vclock_lo, this->ClockParams.one_vclock_hi); //Channel 2: V3

    this->SetDACValueClock(12, this->ClockParams.l_hclock_lo, this->ClockParams.l_hclock_hi); //Channel 12: H1L
    this->SetDACValueClock(13, this->ClockParams.l_hclock_lo, this->ClockParams.l_hclock_hi); //Channel 13: H2L
    this->SetDACValueClock(14, this->ClockParams.l_hclock_lo, this->ClockParams.l_hclock_hi); //Channel 14: H3L
    this->SetDACValueClock(15, this->ClockParams.u_hclock_lo, this->ClockParams.u_hclock_hi); //Channel 15: H1U
    this->SetDACValueClock(16, this->ClockParams.u_hclock_lo, this->ClockParams.u_hclock_hi); //Channel 16: H2U
    this->SetDACValueClock(17, this->ClockParams.u_hclock_lo, this->ClockParams.u_hclock_hi); //Channel 17: H3U

    /*These parameters do change between boards */
    if (this->CCDParams.SecondStageVersion == "UW1") {


        this->SetDACValueClock(6, this->ClockParams.one_tg_lo, this->ClockParams.one_tg_hi); //Channel 6: TG

        this->SetDACValueClock(18, this->ClockParams.one_sw_lo, this->ClockParams.one_sw_hi); //Channel 18: SWL
        this->SetDACValueClock(23, this->ClockParams.one_sw_lo, this->ClockParams.one_sw_hi); //Channel 23: SWU

        //Reset gate needs to be checked against the current timing file and be flipped if necessary
        if (this->CCDParams.InvRG) {
            this->SetDACValueClock(20, this->ClockParams.one_rg_hi, this->ClockParams.one_rg_lo); //Channel 20: RGL
            this->SetDACValueClock(21, this->ClockParams.one_rg_hi, this->ClockParams.one_rg_lo); //Channel 21: RGU
        } else {
            this->SetDACValueClock(20, this->ClockParams.one_rg_lo, this->ClockParams.one_rg_hi); //Channel 20: RGL
            this->SetDACValueClock(21, this->ClockParams.one_rg_lo, this->ClockParams.one_rg_hi); //Channel 21: RGU
        }

        this->SetDACValueClock(7, this->ClockParams.one_og_lo, this->ClockParams.one_og_hi); //Channel 7: OG
        this->SetDACValueClock(9, this->ClockParams.one_og_lo, this->ClockParams.one_og_hi); //Channel 9: OG

        this->SetDACValueClock(8, this->ClockParams.one_dg_lo, this->ClockParams.one_dg_hi); //Channel 8: DG
        this->SetDACValueClock(10, this->ClockParams.one_dg_lo, this->ClockParams.one_dg_hi); //Channel 10: DG



    } else if (this->CCDParams.SecondStageVersion=="UW2") {


        this->SetDACValueClock(3, this->ClockParams.two_vclock_lo, this->ClockParams.two_vclock_hi); //Channel 3: 2V1
        this->SetDACValueClock(4, this->ClockParams.two_vclock_lo, this->ClockParams.two_vclock_hi); //Channel 4: 2V2
        this->SetDACValueClock(5, this->ClockParams.two_vclock_lo, this->ClockParams.two_vclock_hi); //Channel 5: 2V3

        /*You should only be able open the TG corresponding to the direction of charge movement*/
        if (this->CCDParams.VClkDirection == "1") {
            this->SetDACValueClock(6, this->ClockParams.one_tg_lo, this->ClockParams.one_tg_hi); //Channel 6: TG1
            this->SetDACValueClock(8, this->ClockParams.two_tg_hi, this->ClockParams.two_tg_hi); //Channel 8: TG2
        } else if (this->CCDParams.VClkDirection == "2"){
            this->SetDACValueClock(6, this->ClockParams.one_tg_hi, this->ClockParams.one_tg_hi); //Channel 6: TG1
            this->SetDACValueClock(8, this->ClockParams.two_tg_lo, this->ClockParams.two_tg_hi); //Channel 8: TG2
        } else {
            this->SetDACValueClock(6, this->ClockParams.one_tg_lo, this->ClockParams.one_tg_hi); //Channel 6: TG1
            this->SetDACValueClock(8, this->ClockParams.two_tg_lo, this->ClockParams.two_tg_hi); //Channel 8: TG2

            if (this->CCDParams.VClkDirection != "12")
                std::cout<<"V-Clock direction is ambiguous, so both TG are set to enabled. "
                           "However, you should still stop and verify the V-clock directions.";
        }

        if (this->CCDParams.CCDType == "SK"){
            this->SetDACValueClock(7, this->ClockParams.one_og_lo, this->ClockParams.one_og_hi); //Channel 7: OG1
            this->SetDACValueClock(9, this->ClockParams.two_og_lo, this->ClockParams.two_og_hi); //Channel 9: OG2
        } else {
            this->SetDACValueClock(7, this->ClockParams.one_og_hi, this->ClockParams.one_og_hi); //Channel 7: OG1
            this->SetDACValueClock(9, this->ClockParams.two_og_hi, this->ClockParams.two_og_hi); //Channel 9: OG2
        }

        this->SetDACValueClock(18, this->ClockParams.one_sw_lo, this->ClockParams.one_sw_hi); //Channel 18: SWL
        this->SetDACValueClock(23, this->ClockParams.two_sw_lo, this->ClockParams.two_sw_hi); //Channel 23: SWU
        


        //Reset gate needs to be checked against the current timing file and be flipped if necessary
        if (this->CCDParams.InvRG) {
            this->SetDACValueClock(20, this->ClockParams.one_rg_hi, this->ClockParams.one_rg_lo); //Channel 20: RG1
            this->SetDACValueClock(22, this->ClockParams.two_rg_hi, this->ClockParams.two_rg_lo); //Channel 21: RG2
        } else {
            this->SetDACValueClock(20, this->ClockParams.one_rg_lo, this->ClockParams.one_rg_hi); //Channel 20: RG1
            this->SetDACValueClock(22, this->ClockParams.two_rg_lo, this->ClockParams.two_rg_hi); //Channel 21: RG2
        }


        if (this->CCDParams.CCDType == "SK"){
            this->SetDACValueClock(19, this->ClockParams.one_dg_lo, this->ClockParams.one_dg_hi); //Channel 18: DG1
            this->SetDACValueClock(21, this->ClockParams.two_dg_lo, this->ClockParams.two_dg_hi); //Channel 23: DG2
        } else {
            this->SetDACValueClock(19, this->ClockParams.one_dg_lo, this->ClockParams.one_dg_lo); //Channel 18: DG1
            this->SetDACValueClock(21, this->ClockParams.two_dg_lo, this->ClockParams.two_dg_lo); //Channel 23: DG2
        }

    } else {

        std::cout<<"The second stage is neither UW1 or UW2, which means pre-set clock voltages could not be applied. "
                 <<"Stop and check.\n";
    }

}

void LeachController::TurnOffCCDClocks(void )
{
    /*Set Clocks*/

    /*These sets of clocks are common and did not change between Second stage generations.*/

    this->SetDACValueClock(0,0.,0.); //Channel 0: V1
    this->SetDACValueClock(1,0.,0.); //Channel 1: V2
    this->SetDACValueClock(2,0.,0.); //Channel 2: V3

    this->SetDACValueClock(12,0.,0.); //Channel 12: H1L
    this->SetDACValueClock(13,0.,0.); //Channel 13: H2L
    this->SetDACValueClock(14,0.,0.); //Channel 14: H3L
    this->SetDACValueClock(15,0.,0.); //Channel 15: H1U
    this->SetDACValueClock(16,0.,0.); //Channel 16: H2U
    this->SetDACValueClock(17,0.,0.); //Channel 17: H3U


    this->SetDACValueClock(3,0.,0.); //Channel 3: 2V1
    this->SetDACValueClock(4,0.,0.); //Channel 4: 2V2
    this->SetDACValueClock(5,0.,0.); //Channel 5: 2V3


    this->SetDACValueClock(6,0.,0.);//Channel 6: TG1
    this->SetDACValueClock(8,0.,0.); //Channel 8: TG2

    this->SetDACValueClock(7,0.,0.); //Channel 7: OG1
    this->SetDACValueClock(9,0.,0.); //Channel 9: OG2


    this->SetDACValueClock(18,0.,0.); //Channel 18: SWL
    this->SetDACValueClock(23,0.,0.); //Channel 23: SWU

    this->SetDACValueClock(20,0.,0.); //Channel 20: RG1
    this->SetDACValueClock(22,0.,0.); //Channel 21: RG2

    this->SetDACValueClock(19,0.,0.); //Channel 18: DG1
    this->SetDACValueClock(21,0.,0.); //Channel 23: DG2
}

/*
 * Function to apply the bias voltages.
 * Again, this is specific to the UW second stage board.
 */
void LeachController::ApplyAllBiasVoltages(void )
{

    /*Set Biases*/
    //Vdd
    this->SetDACValueBias(0,BiasVoltToADC(this->BiasParams.vdd_1,0));
    this->SetDACValueBias(1,BiasVoltToADC(this->BiasParams.vdd_2,1));
    this->SetDACValueBias(2,0);
    this->SetDACValueBias(3,0);


    //VRef is now set for both SK and DES - there is no need to distinguish them
    this->SetDACValueBias(4,BiasVoltToADC(this->BiasParams.vref_1,4));
    this->SetDACValueBias(5,BiasVoltToADC(this->BiasParams.vref_2,5));

    //Drain1 and Drain2 should always be set. The value to CCD is controlled by the jumper
    this->SetDACValueBias(6,BiasVoltToADC(this->BiasParams.drain_1,6));
    this->SetDACValueBias(7,BiasVoltToADC(this->BiasParams.drain_2,7));


    //OpG for DES CCDs. These could always be set, since the pins are distinct from drain.
    this->SetDACValueBias(8,BiasVoltToADC(this->BiasParams.opg_1,8));
    this->SetDACValueBias(9,BiasVoltToADC(this->BiasParams.opg_2,9));


    this->SetDACValueBias(10,0);
    //Controls Relay for battery box
    this->SetDACValueBias(11,BiasVoltToADC(this->BiasParams.battrelay,11));

    //VSUB
    if(this->BiasParams.useSRSsupply){
	std::cout << "Should set VSub to : " << this->BiasParams.vsub << std::endl;
	this->SetVSUB(this->BiasParams.vsub);
    }

    //Video Offsets, channels 2 and 3 on the video board. 3 is R and 2 is L
    this->SetDACValueVideoOffset(2, this->BiasParams.video_offsets_U);
    this->SetDACValueVideoOffset(3, this->BiasParams.video_offsets_L);

}

void LeachController::TurnOffVSUB(void )
{
    //VSUB - added by KR
    std::cout << "Setting VSub to : 0 V" << std::endl;
    this->SetVSUB(0.);
}

void LeachController::TurnOffBiasVoltages(void )
{

    /*Set Biases*/
    //Vdd
    this->SetDACValueBias(0,0.);
    this->SetDACValueBias(1,0.);
    this->SetDACValueBias(2,0.);
    this->SetDACValueBias(3,0.);


    //VRef 
    this->SetDACValueBias(4,0.);
    this->SetDACValueBias(5,0.);

    //Drain1 and Drain2 
    this->SetDACValueBias(6,0.);
    this->SetDACValueBias(7,0.);


    //OpG for DES CCDs. 
    this->SetDACValueBias(8,0.);
    this->SetDACValueBias(9,0.);


    this->SetDACValueBias(10,0.);
    //Controls Relay for battery box
    this->SetDACValueBias(11,0.);


    //Video Offsets, channels 2 and 3 on the video board. 3 is R and 2 is L
    this->SetDACValueVideoOffset(2,0.);
    this->SetDACValueVideoOffset(3,0.);

}

/*
 * The Vdd toggling function
 */

void LeachController::ToggleVDD(bool VDDState){

    if (VDDState == 1 && !_expose_isVDDOn ){
        this->SetDACValueBias(0,BiasVoltToADC(this->BiasParams.vdd_1,0));
        this->SetDACValueBias(1,BiasVoltToADC(this->BiasParams.vdd_2,1));
        _expose_isVDDOn = true;
    } else if (VDDState == 0 && _expose_isVDDOn){
        this->SetDACValueBias(0,0);
        this->SetDACValueBias(1,0);
        _expose_isVDDOn = false;
    } else if ( (VDDState == 1 && _expose_isVDDOn) || (VDDState == 0 && !_expose_isVDDOn)) {
        printf("VDD state not changed since it is not required.\n");
    } else {
        printf("Could not toggle the VDD since condition was not understood.\n");
    }



}
