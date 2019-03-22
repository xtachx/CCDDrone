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



void LeachController::ParseCCDSettings(CCDVariables &_CCDSettings, ClockVariables &_clockSettings, BiasVariables &_biasSettings)
{

    INIReader _LeachConfig(INIFileLoc.c_str());

    if (_LeachConfig.ParseError() != 0)
    {
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
    if(_CCDSettings.AmplifierDirection == "RL") _CCDSettings.AmplifierDirection = "LR";

    _CCDSettings.HClkDirection = _LeachConfig.Get("ccd", "HClkDirection", "LR");
    if(_CCDSettings.HClkDirection == "RL") _CCDSettings.HClkDirection = "LR";

    _CCDSettings.VClkDirection = _LeachConfig.Get("ccd", "VClkDirection", "1");
    if(_CCDSettings.VClkDirection == "21") _CCDSettings.VClkDirection = "12";

    _CCDSettings.super_sequencer = _LeachConfig.GetBoolean("ccd","super_sequencer", false);
    _CCDSettings.IntegralTime = _LeachConfig.GetReal("ccd","IntegralTime",30.0);


    /*Bias Voltages*/
    _biasSettings.vdd = _LeachConfig.GetReal("bias", "vdd", -22);
    _biasSettings.vrefsk = _LeachConfig.GetReal("bias", "vrefsk", -12.5);
    _biasSettings.vref = _LeachConfig.GetReal("bias", "vref", -13.12);
    _biasSettings.drain = _LeachConfig.GetReal("bias", "drain", -15.26);
    _biasSettings.opg = _LeachConfig.GetReal("bias", "opg", -2.21);
    _biasSettings.battrelay = _LeachConfig.GetReal("bias", "battrelay", -4.88);
    _biasSettings.video_offsets = _LeachConfig.GetInteger("bias", "video_offsets", 0);

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

}


void LeachController::LoadCCDSettingsFresh(void)
{

    this->ParseCCDSettings(this->CCDParams,this->ClockParams,this->BiasParams);
    this->CopyOldAndStoreFileHashes( );
}


