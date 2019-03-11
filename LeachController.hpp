//
// Created by Pitam Mitra on 2019-02-13.
//

#ifndef CCDDRONE_LEACHCONTROLLER_HPP
#define CCDDRONE_LEACHCONTROLLER_HPP

#include "CArcDevice.h"
#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CArcFitsFile.h"
#include "CExpIFace.h"
#include "ArcDefs.h"

#include "BiasReset.hpp"



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


// ------------------------------------------------------
//  Exposure Callback Class - this crazy construction is needed
//  for the Arcapi to work. ?!
// ------------------------------------------------------
class CExposeListener : public arc::device::CExpIFace
{
    void ExposeCallback( float fElapsedTime )
    {
        printf("\rElapsed Time: %.3f",fElapsedTime);
    }
    void ReadCallback( int dPixelCount )
    {
        printf("\rNumber of pixels transferred: %d",dPixelCount);
    }
};




class LeachController {

private:

    arc::device::CArcDevice *pArcDev;
    /*Start the listener object for callback interactions*/
    CExposeListener cExposeListener;

    int ClockVoltToADC( double );
    int BiasVoltToADC(double, int);


    void SetDACValueClock(int , double , double );
    void SetDACValueBias(int , int );
    void ApplyAllPositiveVPixelArray(void );






public:

    LeachController( );

    ~LeachController();

    /*Variables that will need to be set before exposure*/
    std::string INIFileLoc;

    CCDVariables CCDParams;
    ClockVariables ClockParams;
    BiasVariables BiasParams;



    /*Routines*/
    void ParseCCDSettings(CCDVariables&, ClockVariables&, BiasVariables& );
    void ApplyNewCCDSettings(void);
    void CheckForChanges(void );

    void ExposeCCD_DES(int );
    void ExposeCCD_SK(int );
    void ExposeCCD(int, unsigned short*);

    int CheckForSettingsChange(void );
    void CopyOldAndStoreFileHashes(void );

    void ApplyAllCCDClocks(CCDVariables &, ClockVariables & );
    void ApplyAllBiasVoltages(CCDVariables &, BiasVariables & );
    void CCDBiasToggle(bool );

};


#endif //CCDDRONE_LEACHCONTROLLER_HPP
