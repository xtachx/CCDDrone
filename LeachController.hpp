/* *********************************************************************
 * This file is the header that contains all the forward declarations
 * of the routines in the CCDDrone library that can be used by programs
 * to perform operations on the leach controller.
 * The file also contains several definitions of messages to drive the
 * super-sequencer in its different configurations.
 *
 * Created by Pitam Mitra on 2019-02-13.
 * *********************************************************************
 */

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


/*Extra messages implemented in the super-sequencer*/
#define SSR 0x00535352
#define SAT 0x00534154
#define VDR 0x00564452
#define HDR 0x00484452
#define CIT 0x00434954
#define STC 0x00535443
#define CPR 0x00435052
#define CPO 0x0043504F
#define NSB 0x004E5342
#define NPB 0x004E5042



class LeachController
{

private:

    arc::device::CArcPCIe *pArcDev;

    // ------------------------------------------------------
    //  Exposure Callback Class - this crazy construction is needed
    //  for the Arcapi to work. ?!
    // ------------------------------------------------------
    class CExposeListener : public arc::device::CExpIFace
    {
    public:
        LeachController &L;
        CExposeListener(LeachController &LO): L(LO) {};

        void ExposeCallback( float fElapsedTime )
        {
            printf("\rExposure time remaining: %.3f",fElapsedTime);
            /*If the exposure is about to end, and VDD is off, then turn VDD back on*/
            if (L._expose_isVDDOn == false && fElapsedTime < 3.0 ) {
                printf("\nTurning VDD ON\n");
                L.ToggleVDD(1);
            }
        }

        void ReadCallback( int dPixelCount )
        {
            printf("\rNumber of pixels transferred: %d",dPixelCount);
        }
    };

    CExposeListener *cExposeListener;
    //CExposeListener cExposeListener;

    /*LeachControllerClkBiasMethods*/
    int ClockVoltToADC( double );
    int BiasVoltToADC(double, int);
    void SetDACValueClock(int, double, double );
    void SetDACValueBias(int, int );
    void SetDACValueVideoOffset(int, int );

    /*LeachControllerMiscHardwareProcedures - private part*/
    int SetSSR(void );
    int SetCCDType(void );
    int SetVDR(void);
    int SetHDR(void);
    int SelectAmplifierAndHClocks(void);
    int CalculateTiming(double );

    /*LeachControllerExpose - private part*/
    void ExposeCCD( float fExpTime, const bool& bAbort = false,
                    CExposeListener::CExpIFace* pExpIFace = NULL, bool bOpenShutter = true );



public:

    LeachController(std::string );
    ~LeachController();

    /*Variables that will need to be set before exposure*/
    std::string INIFileLoc;
    CCDVariables CCDParams;
    ClockVariables ClockParams;
    BiasVariables BiasParams;
    TimeVariables ClockTimers;

    /*Routines - Universal and defined in LeachController.cpp*/
    void ApplyAllCCDBasic(void );
    /*Routines - UW specific and defined in LeachController.cpp*/
    void ApplyAllCCDClocks(void );
    void ApplyAllBiasVoltages(void );
    void ToggleVDD(bool);


    /*Routines - Generic and organized by filename*/

    /*LeachControllerConfigHandler*/
    void ParseCCDSettings(CCDVariables&, ClockVariables&, BiasVariables& );
    int LoadAndCheckForSettingsChange(bool&, bool& );
    void CopyOldAndStoreFileHashes(void );
    void LoadCCDSettingsFresh(void );


    /*LeachControllerExpose - public part*/
    void PrepareAndExposeCCD(int, unsigned short*);
    int _expose_isVDDOn = true;


    /*LeachControllerMiscHardwareProcedures - public part*/
    void CCDBiasToggle(bool );
    void StartupController(void );
    void PerformEraseProcedure(void);
    void ApplyAllPositiveVPixelArray(void );
    void RestoreVClockVoltages (void);
    void IdleClockToggle(void );
    void ApplyNewSequencer(std::string );
    int ApplyNewIntegralTime(double );
    int ApplyNewPedestalIntegralWait(double );
    int ApplyNewSignalIntegralWait(double);
    int ApplyPBIN(int);
    int ApplySBIN(int);



    /*FitsOps*/
    void SaveFits(std::string );




};


#endif //CCDDRONE_LEACHCONTROLLER_HPP
