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
#include "UtilityFunctions.hpp"
#include "SRSPowerSupplyController.hpp"



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
#define VDXN_1 0x5F5F41
#define VDXN_2 0x5F5F42
#define VDXN_12 0x5F5F43
#define DGW 0x00444757
#define RSW 0x00525357
#define OGW 0x004F4757
#define SWW 0x00535757
#define DRXN_L 0x5F5F4C
#define DRXN_U 0x5F5F55
#define DRXN_LU 0x5F4C55
#define CSL 0x0043534C
#define CSS 0x00435353
#define CPL 0x0043504C
#define CPP 0x00435050
#define HLD 0x00484C44


class LeachController
{

private:

    arc::device::CArcPCIe *pArcDev;
    ProgressBar ReadoutProgress;
    SRSPowerSupplyControler SRSSupply;

    std::string outFileName;
    size_t FrameMemorySize;

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
            std::string VDDState;
            if (L._expose_isVDDOn) {
                VDDState = ColouredFmtText("VDD ON", "green");
            } else {
                VDDState = ColouredFmtText("VDD OFF", "red");
            }
            printf("\rExposure time remaining: %.3f | %s ",fElapsedTime, VDDState.c_str());

            /*If the exposure is about to end, and VDD is off, then turn VDD back on*/
            if (L._expose_isVDDOn == false && fElapsedTime < 3.0 ) {
                L.ToggleVDD(1);
            }
        }

        void ReadCallback( int dPixelCount )
        {
            L.ReadoutProgress.updProgress(dPixelCount+L.TotalPixelsCounted);
            L.ReadoutProgress.display();
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

    // ------------------------------------------------------
    //  Exposure Callback Class - for continuous exposures
    // ------------------------------------------------------
    class CMyConIFace : public arc::device::CConIFace
    {
    public:
        LeachController &L;
        int TotalFramesToRead;

        CMyConIFace( LeachController &LO, int TotalFramesToRead) :  L(LO), TotalFramesToRead(TotalFramesToRead)  {
        };
        ~CMyConIFace() { };

        void ReadCallbackPixel( int dPixelCount )
        {
            L.ReadoutProgress.updProgress(dPixelCount+L.TotalPixelsCounted);
            L.ReadoutProgress.display();
        }


        void FrameCallback( int dFPB, int dCount, int dRows, int dCols, void* pBuf )
        {
            /*Update display*/
            L.ReadoutProgress.updProgressFrame(dCount);

            std::string _thisFrameOutFileName = L.outFileName+"_"+std::to_string(dCount)+".fits";

            /*Set the readout timers*/
            L.SetIntermediateClocks();

            /*De-Interlacing part - If two amplifiers were used, we need to de-interlace*/
            if (L.CCDParams.AmplifierDirection == "UL" || L.CCDParams.AmplifierDirection == "LU") {
                std::cout << "Since amplifier selected was UL / LU, the image will now be de-interlaced.\n";
                unsigned short *pU16Buf = (unsigned short *) pBuf;
                arc::deinterlace::CArcDeinterlace cDlacer;
                int dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_SERIAL;
                cDlacer.RunAlg(pU16Buf, L.CCDParams.dRows, L.CCDParams.dCols * L.CCDParams.nSkipperR, dDeintAlg);
            }

            /*Write the FITS file for this segment*/
            L.SaveFits(_thisFrameOutFileName, pBuf, dCount, TotalFramesToRead, dFPB);
            L.AppendTarball(_thisFrameOutFileName, L.outTarFile);

            /*Re-set the timers*/


            //long _StartColumn = 0;
            //long _StartRow = dCount*RowsPerImageBlock;
            //printf("Adding frame %d\n",dCount);
            //_FitsFile->WriteData(_StartRow, _StartColumn, dRows, dCols, (unsigned short*) pBuf);
        }
    };

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

    /* Added by KR */
    void TurnOffCCDClocks(void );
    void TurnOffBiasVoltages(void ) ;
    void TurnOffVSUB (void ); 

    /*Routines - Generic and organized by filename*/

    /*LeachControllerConfigHandler*/
    void ParseCCDSettings(CCDVariables&, ClockVariables&, BiasVariables& );
    int LoadAndCheckForSettingsChange(bool&, bool& );
    void CopyOldAndStoreFileHashes(void );
    void LoadCCDSettingsFresh(void );


    /*LeachControllerExpose - public part*/
    void PrepareAndExposeCCD(int);
    bool _expose_isVDDOn;



    /*ContinuousExposure*/
    unsigned long TotalPixelsToRead;
    unsigned long TotalPixelsCounted;
    int TotalChunks, CurrentChunk;
    void ExposeContinuous( int dRows, int dCols, int NDCMs, int dNumOfFrames, float fExpTime,
                           const bool& bAbort, CMyConIFace* pConIFace );
    int ContinuousExposeC(int ExposeSeconds, std::string OutFileName, size_t NumContinuousReads);
    void SetIntermediateClocks(void);

    /*LeachControllerMiscHardwareProcedures - public part*/
    void CCDBiasToggle(bool );
    void StartupController(void );
    void PerformEraseProcedure(void);
    void PerformEPurgeProcedure(void);
    void PerformEraseProcedureWithSRS(void);
    void RampDownProcedure(double, double); 
    void RampUpProcedure(double, double); 
    void SetVSUB(double); // Added by KR - for VSub
    void ApplyAllPositiveVPixelArray(void );
    void RestoreVClockVoltages (void);
    void IdleClockToggle(void );
    void ApplyNewSequencer(std::string );
    int ApplyNewIntegralTimeAndGain(double, int );
    int ApplyNewPedestalIntegralWait(double );
    int ApplyNewSignalIntegralWait(double);
    int ApplyPBIN(int);
    int ApplySBIN(int);
    int ApplyGainAndSpeed( int, int );
    int ApplyDGWidth(double);
    int ApplyOGWidth(double);
    int ApplySkippingRGWidth(double);
    int ApplySummingWellWidth(double);
    int ApplyVClockWidths(double , double );
    int ApplyHClockWidths(double , double );
    void ApplyAllPositive(double );

    /*FitsOps*/
    std::string outTarFile;
    void SaveFitsHeader(std::string );
    void SaveFits(std::string outFileName, void* pData, int numFrame = 1, int totalFrames = 1, int FPBCount=0);
    void AppendTarball(std::string fitsFile, std::string TarFile);
    void ArchiveTarball(std::string Tarball);


    /*Access routines - defined here*/
    void* GetCommonBufferVA(void) {return this->pArcDev->CommonBufferVA();};

};


#endif //CCDDRONE_LEACHCONTROLLER_HPP
