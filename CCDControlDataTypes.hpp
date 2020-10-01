//
// Created by Pitam Mitra on 2019-02-13.
//

#ifndef CCDCONTROL_DTYPES
#define CCDCONTROL_DTYPES
#include <chrono>


struct CCDVariables{
    /*Variables that will need to be set before exposure*/
    std::string sTimFile;
    std::string CCDType;
    std::string SecondStageVersion;


    float fExpTime;
    int nSkipperR;
    int dRows;
    int dCols;
    bool InvRG;

    std::string AmplifierDirection;
    std::string HClkDirection;
    std::string VClkDirection;

    bool super_sequencer;
    double IntegralTime;
    double PedestalIntgWait;
    double SignalIntgWait;
    double DGWidth;
    double OGWidth;
    double SKRSTWidth;
    double SWWidth;

    double VWidth;
    double VOverlapWidth;
    double HWidth;
    double HOverlapWidth;

    int ParallelBin;
    int SerialBin;

    int Gain;
    int ItgSpeed;

};

struct ClockVariables{
    /*Clock settings*/
    double one_vclock_hi;
    double one_vclock_lo;
    double two_vclock_hi;
    double two_vclock_lo;

    double one_tg_hi;
    double one_tg_lo;
    double two_tg_hi;
    double two_tg_lo;


    double u_hclock_hi;
    double u_hclock_lo;
    double l_hclock_hi;
    double l_hclock_lo;

    double one_dg_hi;
    double one_dg_lo;
    double two_dg_hi;
    double two_dg_lo;


    double one_rg_hi;
    double one_rg_lo;
    double two_rg_hi;
    double two_rg_lo;


    double one_sw_hi;
    double one_sw_lo;
    double two_sw_hi;
    double two_sw_lo;


    double one_og_hi;
    double one_og_lo;
    double two_og_hi;
    double two_og_lo;

};


struct BiasVariables{

    double vdd_1;
    double vdd_2;
    double vref_1;
    double vref_2;
    double drain_1;
    double drain_2;
    double opg_1;
    double opg_2;
    double battrelay;
    float vsub; 
    float rampdownrate; 
    float rampuprate; 
    float holdtime; 
    float turnoffvoltage; 
    float restartvoltage; 
    bool useSRSsupply;
    int video_offsets_U;
    int video_offsets_L;

};


struct TimeVariables{

    std::chrono::system_clock::time_point ProgramStart;
    std::chrono::system_clock::time_point ExpStart;
    std::chrono::system_clock::time_point Readoutstart;
    std::chrono::system_clock::time_point ReadoutEnd;

    double MeasuredReadout;
    double MeasuredExp;

    bool isExp = false;
    bool isReadout = false;
    bool rClockCounter = 0;

};

#endif //CCDCONTROL_DTYPES
