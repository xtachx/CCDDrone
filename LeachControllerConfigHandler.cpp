//
// Created by Pitam Mitra on 2019-02-13.
//

#include <string>
#include <iostream>
#include <fstream>


#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"
#include "picosha2.h"
#include "INIReader.h"
#include "CCDControlDataTypes.hpp"
#include "SRSPowerSupplyController.hpp"



void LeachController::ParseCCDSettings(CCDVariables &_CCDSettings, ClockVariables &_clockSettings, BiasVariables &_biasSettings)
{

    INIReader _LeachConfig(INIFileLoc.c_str());

    if (_LeachConfig.ParseError() != 0)
    {
        std::cout << "Can't load init file\n";

    }


    _clockSettings.one_vclock_hi = _LeachConfig.GetReal("clocks", "one_vclock_hi", 6);
    _clockSettings.one_vclock_lo = _LeachConfig.GetReal("clocks", "one_vclock_lo", 0);
    _clockSettings.two_vclock_hi = _LeachConfig.GetReal("clocks", "two_vclock_hi", 6);
    _clockSettings.two_vclock_lo = _LeachConfig.GetReal("clocks", "two_vclock_lo", 0);

    _clockSettings.u_hclock_hi = _LeachConfig.GetReal("clocks", "u_hclock_hi", 5);
    _clockSettings.u_hclock_lo = _LeachConfig.GetReal("clocks", "u_hclock_lo", 0);
    _clockSettings.l_hclock_hi = _LeachConfig.GetReal("clocks", "l_hclock_hi", 5);
    _clockSettings.l_hclock_lo = _LeachConfig.GetReal("clocks", "l_hclock_lo", 0);

    _clockSettings.one_tg_hi = _LeachConfig.GetReal("clocks", "one_tg_hi", 6);
    _clockSettings.one_tg_lo = _LeachConfig.GetReal("clocks", "one_tg_lo", 6);
    _clockSettings.two_tg_hi = _LeachConfig.GetReal("clocks", "two_tg_hi", 6);
    _clockSettings.two_tg_lo = _LeachConfig.GetReal("clocks", "two_tg_lo", 6);


    _clockSettings.one_dg_hi = _LeachConfig.GetReal("clocks", "one_dg_hi", 5);
    _clockSettings.one_dg_lo = _LeachConfig.GetReal("clocks", "one_dg_lo", -4);
    _clockSettings.two_dg_hi = _LeachConfig.GetReal("clocks", "two_dg_hi", 5);
    _clockSettings.two_dg_lo = _LeachConfig.GetReal("clocks", "two_dg_lo", -4);


    _clockSettings.one_rg_hi = _LeachConfig.GetReal("clocks", "one_rg_hi", -6);
    _clockSettings.one_rg_lo = _LeachConfig.GetReal("clocks", "one_rg_lo", 0);
    _clockSettings.two_rg_hi = _LeachConfig.GetReal("clocks", "two_rg_hi", -6);
    _clockSettings.two_rg_lo = _LeachConfig.GetReal("clocks", "two_rg_lo", 0);



    _clockSettings.one_sw_hi = _LeachConfig.GetReal("clocks", "one_sw_hi", 4);
    _clockSettings.one_sw_lo = _LeachConfig.GetReal("clocks", "one_sw_lo", -4);
    _clockSettings.two_sw_hi = _LeachConfig.GetReal("clocks", "two_sw_hi", 4);
    _clockSettings.two_sw_lo = _LeachConfig.GetReal("clocks", "two_sw_lo", -4);



    _clockSettings.one_og_hi = _LeachConfig.GetReal("clocks", "one_og_hi", 2);
    _clockSettings.one_og_lo = _LeachConfig.GetReal("clocks", "one_og_lo", -2);
    _clockSettings.two_og_hi = _LeachConfig.GetReal("clocks", "two_og_hi", 2);
    _clockSettings.two_og_lo = _LeachConfig.GetReal("clocks", "two_og_lo", -2);



    /*CCD Specific settings*/
    _CCDSettings.CCDType = _LeachConfig.Get("ccd", "type", "DES");
    _CCDSettings.dCols = _LeachConfig.GetInteger("ccd", "columns", 4000);
    _CCDSettings.dRows = _LeachConfig.GetInteger("ccd", "rows", 4000);
    _CCDSettings.nSkipperR = _LeachConfig.GetInteger("ccd", "NDCM", 1);
    _CCDSettings.sTimFile = _LeachConfig.Get("ccd", "sequencer_loc", "tim_current.lod");
    _CCDSettings.SecondStageVersion = _LeachConfig.Get("ccd", "second_stage", "UW1");
    _CCDSettings.InvRG = _LeachConfig.GetBoolean("ccd", "RG_inv", true);

    _CCDSettings.AmplifierDirection = _LeachConfig.Get("ccd", "AmplifierDirection", "UL");
    if(_CCDSettings.AmplifierDirection == "LU") _CCDSettings.AmplifierDirection = "UL";

    _CCDSettings.HClkDirection = _LeachConfig.Get("ccd", "HClkDirection", "UL");
    if(_CCDSettings.HClkDirection == "LU") _CCDSettings.HClkDirection = "UL";

    _CCDSettings.VClkDirection = _LeachConfig.Get("ccd", "VClkDirection", "1");
    if(_CCDSettings.VClkDirection == "21") _CCDSettings.VClkDirection = "12";

    _CCDSettings.super_sequencer = _LeachConfig.GetBoolean("ccd","super_sequencer", false);


    _CCDSettings.ParallelBin =  _LeachConfig.GetInteger("ccd","ParallelBin",1);
    _CCDSettings.SerialBin =  _LeachConfig.GetInteger("ccd","SerialBin",1);

    int ccd_gain = _LeachConfig.GetInteger("ccd","Gain",1);
    if (ccd_gain == 1 || ccd_gain == 2 || ccd_gain == 5 || ccd_gain == 10){
        _CCDSettings.Gain = ccd_gain;
    } else {
        std::cout<<"Warning: The gain must be 1,2,5 or 10. Setting the gain to 1.\n";
        _CCDSettings.Gain = 1;
    }


    /*Timing Settings*/
    _CCDSettings.IntegralTime = _LeachConfig.GetReal("timing","IntegralTime",30.0);
    _CCDSettings.PedestalIntgWait =  _LeachConfig.GetReal("timing","PedestalIntgWait",2.0);
    _CCDSettings.SignalIntgWait =  _LeachConfig.GetReal("timing","SignalIntgWait",0.5);
    _CCDSettings.DGWidth =  _LeachConfig.GetReal("timing","DGWidth",0.6);
    _CCDSettings.OGWidth =  _LeachConfig.GetReal("timing","OGWidth",0.6);
    _CCDSettings.SKRSTWidth =  _LeachConfig.GetReal("timing","RGWidth",0.6);
    _CCDSettings.SWWidth =  _LeachConfig.GetReal("timing","SWPulseWidth",0.6);
    _CCDSettings.VWidth =  _LeachConfig.GetReal("timing","VWidth",20);
    _CCDSettings.VOverlapWidth =  _LeachConfig.GetReal("timing","VOverlapWidth",10);
    _CCDSettings.HWidth =  _LeachConfig.GetReal("timing","HWidth",0.6);
    _CCDSettings.HOverlapWidth =  _LeachConfig.GetReal("timing","HOverlapWidth",0.6);




    /*Bias Voltages*/
    _biasSettings.vdd_1 = _LeachConfig.GetReal("bias", "vdd_1", -22);
    _biasSettings.vdd_2 = _LeachConfig.GetReal("bias", "vdd_2", -22);
    _biasSettings.vref_1 = _LeachConfig.GetReal("bias", "vref_1", -12.5);
    _biasSettings.vref_2 = _LeachConfig.GetReal("bias", "vref_2", -12.5);
    _biasSettings.drain_1 = _LeachConfig.GetReal("bias", "drain_1", -15.26);
    _biasSettings.drain_2 = _LeachConfig.GetReal("bias", "drain_2", -15.26);
    _biasSettings.opg_1 = _LeachConfig.GetReal("bias", "opg_1", -2.21);
    _biasSettings.opg_2 = _LeachConfig.GetReal("bias", "opg_2", -2.21);
    _biasSettings.battrelay = _LeachConfig.GetReal("bias", "battrelay", -4.88);
    _biasSettings.vsub = _LeachConfig.GetReal("bias", "vsub", 45.);  
    _biasSettings.rampdownrate = _LeachConfig.GetReal("bias","ramp_down_rate",15); 
    _biasSettings.rampuprate = _LeachConfig.GetReal("bias","ramp_up_rate",45);     
    _biasSettings.holdtime = _LeachConfig.GetReal("bias","hold_vsuboff_seconds",0.5); 
    _biasSettings.restartvoltage = _LeachConfig.GetReal("bias","restart_clock_voltage",10); 
    _biasSettings.turnoffvoltage = _LeachConfig.GetReal("bias","turnoff_clock_voltage",10); 
    _biasSettings.useSRSsupply   =  _LeachConfig.GetBoolean("bias", "use_srs_supply", false );
    _biasSettings.SRSSerialPort  =  _LeachConfig.Get("bias", "srs_serial_port", "/dev/ttyUSB0");
    _biasSettings.video_offsets_U = _LeachConfig.GetInteger("bias", "video_offsets_U", 0);
    _biasSettings.video_offsets_L = _LeachConfig.GetInteger("bias", "video_offsets_L", 0);

    if(_biasSettings.useSRSsupply){
        SRSSupply = SRSPowerSupplyController(_biasSettings.SRSSerialPort);
    }else{
        SRSSupply = SRSPowerSupplyController();
    }
}


int LeachController::LoadAndCheckForSettingsChange(bool &config, bool &sequencer )
{

    /*Load the old SHA256 keys*/
    std::ifstream f3("do_not_touch/LastHashes.txt", std::fstream::in);
    std::string OldSettingsHash;
    std::string OldFirmwareHash;

    std::getline(f3, OldSettingsHash);
    std::getline(f3, OldFirmwareHash);

    /*Load the new settings*/
    this->ParseCCDSettings(this->CCDParams,this->ClockParams,this->BiasParams);

    /*Calculate new SHA256 keys*/
    std::ifstream f1(this->INIFileLoc, std::fstream::binary);
    std::vector<unsigned char> s1(picosha2::k_digest_size);
    picosha2::hash256(f1, s1.begin(), s1.end());
    f1.close();

    std::ifstream fSeq(this->CCDParams.sTimFile, std::fstream::binary);
    std::vector<unsigned char> s2(picosha2::k_digest_size);
    picosha2::hash256(fSeq, s2.begin(), s2.end());
    fSeq.close();

    /*Compare keys and return based on the match conditions*/
    std::string f1s = picosha2::bytes_to_hex_string(s1.begin(), s1.end());
    std::string f2s = picosha2::bytes_to_hex_string(s2.begin(), s2.end());
    config = 0;
    sequencer = 0;

    if (f1s != OldSettingsHash)
    {
        //std::cout << "You have changed the settings, but did not apply them to the controller.\n";
        config = 1;

    }

    if (f2s != OldFirmwareHash)
    {
        //std::cout << "You have changed the firmware, but did not upload it to the leach.\n";
        sequencer = 1;
    }

    if (config | sequencer) return -1;
    else return 0;

}

void LeachController::CopyOldAndStoreFileHashes(void)
{

    //Copy the settings file first for later comparisons.
    std::ifstream f1(this->INIFileLoc, std::fstream::binary);
    std::ofstream f2("do_not_touch/LastSettings.ini", std::fstream::trunc);
    f2 << f1.rdbuf();

    f1.clear();
    f1.seekg(0);


    std::ifstream fSeq(this->CCDParams.sTimFile, std::ios::binary);
    std::vector<unsigned char> s1(picosha2::k_digest_size);
    picosha2::hash256(f1, s1.begin(), s1.end());
    std::vector<unsigned char> s2(picosha2::k_digest_size);
    picosha2::hash256(fSeq, s2.begin(), s2.end());


    std::string f1s = picosha2::bytes_to_hex_string(s1.begin(), s1.end());
    std::string f2s = picosha2::bytes_to_hex_string(s2.begin(), s2.end());

    std::ofstream f3("do_not_touch/LastHashes.txt", std::fstream::trunc | std::fstream::out);
    f3 << f1s << "\n" << f2s;
    f3.close();

    std::ofstream f4("do_not_touch/LastConfigLocation.txt", std::fstream::trunc | std::fstream::out);
    f4 << this->INIFileLoc << "\n";
    f4.close();

}


void LeachController::LoadCCDSettingsFresh(void)
{

    this->ParseCCDSettings(this->CCDParams,this->ClockParams,this->BiasParams);
    this->CopyOldAndStoreFileHashes( );
}
