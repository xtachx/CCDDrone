//
// Created by Pitam Mitra on 2019-02-13.
//

#include "LeachController.hpp"
#include "INIReader.h"
#include <string>
#include <iostream>
#include <fstream>

#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CArcFitsFile.h"
#include "CExpIFace.h"
#include "ArcDefs.h"

#include "picosha2.h"


/*Click driver jumper setting*/
#define CLOCK_JUMPER 2

/* Clock driver max volts +10 and min volts -10*/
#define MAX_CLOCK 13.0
#define MIN_CLOCK -13.0




LeachController::LeachController() {

    /*New ArcDevice*/
    pArcDev = new arc::device::CArcPCIe;

    /*Open the Arc controller by loading a timing file*/
    arc::device::CArcPCIe::FindDevices();
    pArcDev->Open(0);


}

LeachController::~LeachController() {

    pArcDev->Close();

}


void LeachController::ParseCCDSettings(CCDVariables &_CCDSettings, ClockVariables &_clockSettings, BiasVariables &_biasSettings) {

    INIReader _LeachConfig(INIFileLoc.c_str());

    if (_LeachConfig.ParseError() != 0) {
        std::cout << "Can't load init file\n";

    }


    _clockSettings.vclock_hi = _LeachConfig.GetReal("clocks", "vclock_hi", 6);
    _clockSettings.vclock_lo = _LeachConfig.GetReal("clocks", "vclock_lo", 0);

    _clockSettings.u_hclock_hi = _LeachConfig.GetReal("clocks", "u_hclock_hi", 5);
    _clockSettings.u_hclock_lo = _LeachConfig.GetReal("clocks", "u_hclock_lo", 0);
    _clockSettings.l_hclock_hi = _LeachConfig.GetReal("clocks", "l_hclock_hi", 5);
    _clockSettings.l_hclock_lo = _LeachConfig.GetReal("clocks", "l_hclock_lo", 0);

    _clockSettings.tg_hi = _LeachConfig.GetReal("clocks", "tg_hi", 6);
    _clockSettings.tg_lo = _LeachConfig.GetReal("clocks", "tg_lo", 6);

    _clockSettings.dg_hi = _LeachConfig.GetReal("clocks", "dg_hi", 5);
    _clockSettings.dg_lo = _LeachConfig.GetReal("clocks", "dg_lo", -4);
    _clockSettings.rg_hi = _LeachConfig.GetReal("clocks", "rg_hi", -6);
    _clockSettings.rg_lo = _LeachConfig.GetReal("clocks", "rg_lo", 0);

    _clockSettings.sw_hi = _LeachConfig.GetReal("clocks", "sw_hi", 4);
    _clockSettings.sw_lo = _LeachConfig.GetReal("clocks", "sw_lo", -4);
    _clockSettings.og_hi = _LeachConfig.GetReal("clocks", "og_hi", 2);
    _clockSettings.og_lo = _LeachConfig.GetReal("clocks", "og_lo", -2);

    /*CCD Specific settings*/
    _CCDSettings.CCDType = _LeachConfig.Get("ccd", "type", "DES");
    _CCDSettings.dCols = _LeachConfig.GetInteger("ccd", "columns", 4000);
    _CCDSettings.dRows = _LeachConfig.GetInteger("ccd", "rows", 4000);
    _CCDSettings.nSkipperR = _LeachConfig.GetInteger("ccd", "NDCM", 1);
    _CCDSettings.sTimFile = _LeachConfig.Get("ccd", "sequencer_loc", "tim_current.lod");
    _CCDSettings.InvRG = _LeachConfig.GetBoolean("ccd", "RG_inv", true);

    _CCDSettings.AmplifierDirection = _LeachConfig.Get("ccd", "AmplifierDirection", "LR");
    _CCDSettings.HClkDirection = _LeachConfig.Get("ccd", "HClkDirection", "LR");
    _CCDSettings.VClkDirection = _LeachConfig.Get("ccd", "VClkDirection", "NORM");


    /*Bias Voltages*/
    _biasSettings.vdd = _LeachConfig.GetReal("bias", "vdd", -22);
    _biasSettings.vrefsk = _LeachConfig.GetReal("bias", "vrefsk", -12.5);
    _biasSettings.vref = _LeachConfig.GetReal("bias", "vref", -13.12);
    _biasSettings.drain = _LeachConfig.GetReal("bias", "drain", -15.26);
    _biasSettings.opg = _LeachConfig.GetReal("bias", "opg", -2.21);
    _biasSettings.battrelay = _LeachConfig.GetReal("bias", "battrelay", -4.88);
    _biasSettings.video_offsets = _LeachConfig.GetInteger("bias", "video_offsets", 0);

}


int LeachController::CheckForSettingsChange(void) {

    std::ifstream f3("do_not_touch/LastHashes.txt", std::fstream::in);
    std::string OldSettingsHash;
    std::string OldFirmwareHash;

    std::getline(f3, OldSettingsHash);
    std::getline(f3, OldFirmwareHash);


    std::ifstream f1(this->INIFileLoc, std::fstream::binary);
    std::ifstream fSeq(this->CCDParams.sTimFile, std::ios::binary);
    std::vector<unsigned char> s1(picosha2::k_digest_size);
    picosha2::hash256(f1, s1.begin(), s1.end());
    std::vector<unsigned char> s2(picosha2::k_digest_size);
    picosha2::hash256(fSeq, s2.begin(), s2.end());

    std::string f1s(s1.begin(), s1.end());
    std::string f2s(s2.begin(), s2.end());


    if (f1s != OldSettingsHash) {
        std::cout << "You have changed the settings, but did not apply them to the controller.\n";
        return -1;
    }

    if (f2s != OldFirmwareHash) {
        std::cout << "You have changed the firmware, but did not upload it to the leach.\n";
        return -2;
    }

    return 0;


}

void LeachController::CopyOldAndStoreFileHashes(void) {

    //Copy the settings file first for later comparisons.
    std::ifstream f1(this->INIFileLoc, std::fstream::binary);
    std::ofstream f2("do_not_touch/LastSettings.ini", std::fstream::trunc | std::fstream::binary);
    f2 << f1.rdbuf();


    std::ifstream fSeq(this->CCDParams.sTimFile, std::ios::binary);
    std::vector<unsigned char> s1(picosha2::k_digest_size);
    picosha2::hash256(f1, s1.begin(), s1.end());
    std::vector<unsigned char> s2(picosha2::k_digest_size);
    picosha2::hash256(fSeq, s2.begin(), s2.end());

    std::string f1s(s1.begin(), s1.end());
    std::string f2s(s2.begin(), s2.end());

    std::ofstream f3("do_not_touch/LastHashes.txt", std::fstream::trunc | std::fstream::out);
    f3 << f1s << "\n" << f2s;


}


void LeachController::ExposeCCD_DES(int ExposeTime) {

    int ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows * sizeof(short);
    pArcDev->MapCommonBuffer(ImageMemorySize);

    //Select amplifiers and de-interlacing
    int dDeintAlg, SOS_response;
    if (this->CCDParams.AmplifierDirection == "LR") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_PARALLEL;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_LR);

    } else if (this->CCDParams.AmplifierDirection == "L") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_L);

    } else if (this->CCDParams.AmplifierDirection == "R") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_R);

    } else {
        std::cout << "The amplifier selected does not exist. Stop and verify!\n";
    }

    if (SOS_response != DON)
        std::cout << "Amplifier settings could not be applied. \n";
    else
        std::cout << "Amplifier selected. \n";

    std::cout << "Starting exposure\n";
    pArcDev->Expose(ExposeTime, this->CCDParams.dRows, this->CCDParams.dCols, false, &this->cExposeListener);


    if (this->CCDParams.AmplifierDirection == "LR") {
        std::cout << "Since amplifier selected was LR, the image will now be de-interlaced.\n";
        arc::deinterlace::CArcDeinterlace cDlacer;
        unsigned short *pU16Buf = (unsigned short *) pArcDev->CommonBufferVA();
        cDlacer.RunAlg(pU16Buf, this->CCDParams.dRows, this->CCDParams.dCols, dDeintAlg);
    }

}


void LeachController::ExposeCCD_SK(int ExposeTime) {

    int ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows * this->CCDParams.nSkipperR * sizeof(short);
    //Implement a memory size check here.
    pArcDev->MapCommonBuffer(ImageMemorySize);

    //Select amplifiers and de-interlacing
    int dDeintAlg, SOS_response;
    if (this->CCDParams.AmplifierDirection == "LR") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_PARALLEL;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_LR);

    } else if (this->CCDParams.AmplifierDirection == "L") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_L);

    } else if (this->CCDParams.AmplifierDirection == "R") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_R);

    } else {
        std::cout << "The amplifier selected does not exist. Stop and verify!\n";
    }

    if (SOS_response != DON)
        std::cout << "Amplifier settings could not be applied. \n";
    else
        std::cout << "Amplifier selected. \n";

    std::cout << "Starting exposure\n";
    pArcDev->Expose(ExposeTime, this->CCDParams.dRows, this->CCDParams.dCols * this->CCDParams.nSkipperR, false,
                    &this->cExposeListener);


    if (this->CCDParams.AmplifierDirection == "LR") {
        unsigned short *pU16Buf = (unsigned short *) pArcDev->CommonBufferVA();
        std::cout << "Since amplifier selected was LR, the image will now be de-interlaced.\n";
        arc::deinterlace::CArcDeinterlace cDlacer;
        cDlacer.RunAlg(pU16Buf, this->CCDParams.dRows, this->CCDParams.dCols * this->CCDParams.nSkipperR, dDeintAlg);
    }


}

void LeachController::ExposeCCD(int ExposureTime, unsigned short *ImageBuffer) {

    try {
        if (this->CCDParams.CCDType == "SK") this->ExposeCCD_SK(ExposureTime);
        else this->ExposeCCD_DES(ExposureTime);

        ImageBuffer = (unsigned short *) this->pArcDev->CommonBufferVA();

    }
    catch (std::runtime_error &e) {
        std::cout << "failed!" << std::endl;
        std::cerr << std::endl << e.what() << std::endl;

        if (pArcDev->IsReadout()) {
            pArcDev->StopExposure();
        }

        //pArcDev->Close();
    }
    catch (...) {
        std::cerr << std::endl << "Error: unknown exception occurred!!!" << std::endl;

        if (pArcDev->IsReadout()) {
            pArcDev->StopExposure();
        }

        //pArcDev->Close();
    }


}

void LeachController::ApplyAllCCDClocks(CCDVariables &_CCDSettings, ClockVariables &_clockSettings){


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
    if (_CCDSettings.InvRG){
        this->SetDACValueClock(20, _clockSettings.rg_hi, _clockSettings.rg_lo); //Channel 20: RGL
        this->SetDACValueClock(21, _clockSettings.rg_hi, _clockSettings.rg_lo); //Channel 21: RGU
    } else {
        this->SetDACValueClock(20, _clockSettings.rg_lo, _clockSettings.rg_hi); //Channel 20: RGL
        this->SetDACValueClock(21, _clockSettings.rg_lo, _clockSettings.rg_hi); //Channel 21: RGU
    }

}


void LeachController::ApplyAllBiasVoltages(CCDVariables &_CCDSettings, BiasVariables &_BiasSettings ){

    /*Set Biases*/
    //Vdd
    this->SetDACValueBias(0,BiasVoltToADC(_BiasSettings.vdd,0));
    this->SetDACValueBias(1,BiasVoltToADC(_BiasSettings.vdd,1));
    this->SetDACValueBias(2,0);
    this->SetDACValueBias(3,0);


    //VR(1-4)
    if (_CCDSettings.CCDType=="DES"){
        this->SetDACValueBias(4,BiasVoltToADC(_BiasSettings.vref,4));
        this->SetDACValueBias(5,BiasVoltToADC(_BiasSettings.vref,5));
    } else {
        this->SetDACValueBias(4,BiasVoltToADC(_BiasSettings.vrefsk,4));
        this->SetDACValueBias(5,BiasVoltToADC(_BiasSettings.vrefsk,5));
    }
    //DrainL and DrainU
    if(_CCDSettings.CCDType=="SK"){
        this->SetDACValueBias(6,BiasVoltToADC(_BiasSettings.drain,6));
        this->SetDACValueBias(7,BiasVoltToADC(_BiasSettings.drain,7));
    }

    //OG(1-4)
    if (_CCDSettings.CCDType=="DES"){
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


int LeachController::ClockVoltToADC(double PD){

    int clkVoltADC;
    double ADCVal = 4095.0*(PD-MIN_CLOCK)/(MAX_CLOCK-MIN_CLOCK);
    clkVoltADC = (int)ADCVal;
    return 0x00000FFF & clkVoltADC;
}

int LeachController::BiasVoltToADC(double PD, int line){

    int biasVoltADC;

    //Line 0-7 have 25V reference, lines 8-12 have a 5V reference **CHECK**!!
    if (line<8) ADCval = PD*4095/25.0;
    else ADCVal = PD*4095/5.0;

    biasVoltADC = (int)ADCVal;
    if (biasVoltADC > 4095) printf ("Warning: Bias on line %d is %d which is more than the limit 4095.\n ",line,biasVoltADC);
    return 0x00000FFF & biasVoltADC;

}

void LeachController::SetDACValueClock(int dac_chan, double dmin, double dmax){



    int resp1, resp2;

    resp1 = this->pArcDev->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*dac_chan, CLK, ClockVoltToADC(dmax) ); //MAX
    resp2 = this->pArcDev->Command( TIM_ID, SBN, CLOCK_JUMPER,  2*dac_chan+1, CLK, ClockVoltToADC(dmin) ); //MIN

    if (resp1 != 0x00444F4E || resp2 != 0x00444F4E )
        printf ("Error setting CVIon channel: %d | code (max, min): (%X, %X)\n", dac_chan, resp1, resp2);

    //printf ("Written: %X, %X, %d, %d , %X, %d\n", TIM_ID, SBN, CLOCK_JUMPER, 2*DAC, CLK, ClockVoltToADC(dmax));
}


void LeachController::SetDACValueBias(int dac_chan, int val){

    int resp;

    resp = this->pArcDev->Command( TIM_ID, SBN, CLOCK_JUMPER, dac_chan, VID, val ); //MAX

    if (resp != 0x00444F4E )
        printf ("Error setting CVIon channel: %d | code: %X\n", dac_chan, resp);

    //printf ("Written: %X, %X, %d, %d , %X, %d\n", TIM_ID, SBN, CLOCK_JUMPER, dac_chan, VID, val);

}

void LeachController::ApplyAllPositiveVPixelArray(){

    this->SetDACValueClock(0, 9.0, 9.0); //Channel 0: Min 9V Max 9V
    this->SetDACValueClock(1, 9.0, 9.0); //Channel 1: Min 9V Max 9V
    this->SetDACValueClock(2, 9.0, 9.0); //Channel 2: Min 9V Max 9V

}


void LeachController::CCDBiasToggle (bool state){

    if (state) this->SetDACValueBias(11,BiasVoltToADC(_BiasSettings.battrelay,11));
    else this->SetDACValueBias(11,0);

}
