//
// Created by Pitam Mitra on 2019-02-13.
//

#ifndef CCDDRONE_LEACHCONTROLLER_HPP
#define CCDDRONE_LEACHCONTROLLER_HPP

#include <string>

#include "CArcDevice.h"
#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CExpIFace.h"
#include "ArcDefs.h"

#include "CCDControlDataTypes.hpp"


#define SSR 0x00535352
#define SAT 0x00534154
#define VDR 0x00564452
#define HDR 0x00484452
#define CIT 0x00434954



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

    /*LeachControllerClkBiasMethods*/
    int ClockVoltToADC( double );
    int BiasVoltToADC(double, int);
    void SetDACValueClock(int , double , double );
    void SetDACValueBias(int , int );
    void SetDACValueVideoOffset(int, int );

    /*LeachControllerMiscHardwareProcedures - private part*/
    int SetSSR(void );
    int SetCCDType(void );
    int SetVDR(void);
    int SetHDR(void);
    int SelectAmplifierAndHClocks(void);
    /*LeachControllerExpose - private part*/
    int ExposeCCD(int );


public:

    LeachController(std::string );
    ~LeachController();

    /*Variables that will need to be set before exposure*/
    std::string INIFileLoc;
    CCDVariables CCDParams;
    ClockVariables ClockParams;
    BiasVariables BiasParams;

    /*Routines - Universal and defined in LeachController.cpp*/
    void ApplyAllCCDBasic(void );
    /*Routines - UW specific and defined in LeachController.cpp*/
    void ApplyAllCCDClocks(void );
    void ApplyAllBiasVoltages(void );


    /*Routines - Generic and organized by filename*/

    /*LeachControllerConfigHandler*/
    void ParseCCDSettings(CCDVariables&, ClockVariables&, BiasVariables& );
    int LoadAndCheckForSettingsChange(bool&, bool& );
    void CopyOldAndStoreFileHashes(void );
    void LoadCCDSettingsFresh(void );


    /*LeachControllerExpose - public part*/
    void PrepareAndExposeCCD(int, unsigned short*);


    /*LeachControllerMiscHardwareProcedures - public part*/
    void CCDBiasToggle(bool );
    void StartupController(void );
    void PerformEraseProcedure(void);
    void ApplyAllPositiveVPixelArray(void );
    void RestoreVClockVoltages (void);
    void IdleClockToggle(void );
    void ApplyNewSequencer(std::string );
    int ApplyNewIntegralTime(double );


    /*FitsOps*/
    void SaveFits(std::string );




};


#endif //CCDDRONE_LEACHCONTROLLER_HPP
