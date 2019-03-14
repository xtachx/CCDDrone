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
#include "CExpIFace.h"
#include "ArcDefs.h"

#include "CCDControlDataTypes.hpp"



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

    void SaveFits(std::string );




public:

    LeachController(std::string );

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

    int LoadAndCheckForSettingsChange(void );
    void CopyOldAndStoreFileHashes(void );

    void ApplyAllCCDClocks(CCDVariables &, ClockVariables & );
    void ApplyAllBiasVoltages(CCDVariables &, BiasVariables & );
    void CCDBiasToggle(bool );
    void StartupController(void );

    void ApplyAllPositiveVPixelArray(void );
    void RestoreVClockVoltages (void);
    void IdleClockToggle(void );

    void LoadCCDSettingsFresh(void );


};


#endif //CCDDRONE_LEACHCONTROLLER_HPP
