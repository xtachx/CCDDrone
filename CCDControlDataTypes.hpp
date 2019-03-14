//
// Created by Pitam Mitra on 2019-02-13.
//

#ifndef CCDCONTROL_DTYPES
#define CCDCONTROL_DTYPES



struct CCDVariables{
    /*Variables that will need to be set before exposure*/
    std::string sTimFile;
    std::string CCDType;
    float fExpTime;
    int nSkipperR;
    int dRows;
    int dCols;
    bool InvRG;

    std::string AmplifierDirection;
    std::string HClkDirection;
    std::string VClkDirection;

};

struct ClockVariables{
    /*Clock settings*/
    double vclock_hi;
    double vclock_lo;

    double tg_hi;
    double tg_lo;

    double u_hclock_hi;
    double u_hclock_lo;
    double l_hclock_hi;
    double l_hclock_lo;

    double dg_hi;
    double dg_lo;
    double rg_hi;
    double rg_lo;

    double sw_hi;
    double sw_lo;
    double og_hi;
    double og_lo;

};


struct BiasVariables{

    double vdd;
    double vrefsk;
    double vref;
    double drain;
    double opg;
    double battrelay;
    int video_offsets;

};



#endif //CCDCONTROL_DTYPES
