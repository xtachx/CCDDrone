/* *********************************************************************
 * This is the continuous exposure routine for the Leach system. This
 * is a part of the CCDDrone suites to run the Leach system with the
 * "SuperSequencer" allowing for the ability to run Skipper CCDs.
 * The default pinouts use the UW configuration, but are changeable.
 *
 * Designed and written by Pitam Mitra,
 * at the University of Washington for the DAMIC-M collaboration.
 *
 *
 * *********************************************************************
 */

#include <chrono>
#include <iostream>
#include <string>

#include "ArcDefs.h"
#include "CArcDeinterlace.h"
#include "CArcDevice.h"
#include "CArcPCI.h"
#include "CArcPCIe.h"
#include "CConIFace.h"
#include "CExpIFace.h"
#include "FitsOps.hpp"
#include "LeachController.hpp"
#include "UtilityFunctions.hpp"

//#define MaxDataPerFrame 1073741824
//#define MaxDataPerFrame 536870912
#define MaxDataPerFrame 524288000
#define NumRowsInContReadout 20

#define OverrideExposeStrategy 1
#define OverrideStrategyNumber 3

/* This "abort exposure" condition seems to get executed a lot
 * so its better to use the preprocessor to reduce clutter
 */

#define ChkAbortExposure                         \
  if (bAbort) {                                  \
    pArcDev->StopExposure();                     \
    throw std::runtime_error("Expose Aborted!"); \
  }





int LeachController::ContinuousExposeC(int ExposeSeconds,
                                       std::string OutFileName, size_t NumContinuousReads)
{

    /*ContinuousExposure should be in blocks. Its better if each image size is
     *half the total allowable frame size, so lets calculate this first
     *and set it here.
     */

    size_t RowsPerImageBlock;
    size_t ReminderRows;

    /*Total pixels to read (per frame) is cols*rows*NDCMs*/
    this->TotalPixelsToRead = this->CCDParams.dCols * this->CCDParams.dRows * this->CCDParams.nSkipperR;
    /*The memory required for reading a full frame is numPixels*2 (actually sizeof(short)) since we have 2 bytes/pixel*/
    size_t ImageMemorySize = this->TotalPixelsToRead * sizeof(unsigned short);



    /*Each "line" in the image will have these many pixels*/
    size_t TotalCol = this->CCDParams.dCols * this->CCDParams.nSkipperR;
    /*Size of each row in bytes*/
    size_t SingleRow = TotalCol * sizeof(unsigned short);


    /*Parameters needed for stat tracking*/
    this->TotalPixelsCounted = 0;
    //this->ReadoutProgress.SetEssentials(this->TotalPixelsToRead, this->ClockTimers.Readoutstart, NumContinuousReads);



    /*Text display info*/
    std::string ExposeStrategyTxt = "CCDDExposeC (continuous readout)";
    ExposeStrategyTxt = ColouredFmtText(ExposeStrategyTxt, "magenta");

    std::string ExposeStrategyExtra = std::to_string(NumContinuousReads);
    ExposeStrategyExtra = ColouredFmtText(ExposeStrategyExtra, "red", "bold");

    std::cout << ExposeStrategyTxt << " | Number of continuous reads: " << ExposeStrategyExtra << "\n";
    this->TotalChunks = NumContinuousReads;


    /*FITS file storage initialization*/
    FitsOps FITSImage(OutFileName, this->CCDParams, this->BiasParams, this->ClockParams, this->ClockTimers);
    /*Set up the FITS file header*/
    FITSImage.WriteHeader();



    /*Perform readout*/
    //this->_FitsFile = &FITSImage; //This is for cConIface to modify the fits file.
    CMyConIFace cMyConIFace(*this, NumContinuousReads, &FITSImage);



    this->ExposeContinuous(NumRowsInContReadout, this->CCDParams.dCols, this->CCDParams.nSkipperR,
                           NumContinuousReads, this->CCDParams.fExpTime,
                           false,
                           &cMyConIFace);

    /*Write the post exposure info*/
    FITSImage.WritePostExposureInfo();

    return 0;
}




// +----------------------------------------------------------------------------
// |  ExposeContinuous
// +----------------------------------------------------------------------------
// |  This method can be called to start continuous readout.  A callback for
// |  each frame read out can be used to process the frame.
// |
// |  Throws std::runtime_error on error
// |
// |  <IN> -> dRows - The image row size ( in pixels ).
// |  <IN> -> dCols - The image column size ( in pixels ).
// |  <IN> -> dNumOfFrames - The number of frames to take.
// |  <IN> -> fExpTime - The exposure time ( in seconds ).
// |  <IN> -> bAbort - 'true' to cause the readout method to abort/stop either
// |                    exposing or image readout. Default: false
// |  <IN> -> pConIFace - Function pointer to callback for frame completion.
// |                       NULL by default.
// +----------------------------------------------------------------------------
void LeachController::ExposeContinuous(int dRows, int dCols, int NDCMs, int dNumOfFrames, float fExpTime,
                                       const bool& bAbort, CMyConIFace* pConIFace)
{
    int dFramesPerBuffer = 0;
    int dPCIFrameCount = 0;
    int dLastPCIFrameCount = 0;
    int dFPBCount = 0;

    float fRemainingTime = fExpTime;
    bool bInReadout = false;
    bool bLastInReadoutState = false;
    int dTimeoutCounter = 0;
    int dLastPixelCount = 0;
    int dPixelCount = 0;
    int dExposeCounter = 0;

    int dRetVal;

    /*Number of pixels to read in each frame*/
    size_t dExpectedPixelsEachFrame = dRows * dCols * NDCMs;
    size_t dImageSizeEachFrame = dExpectedPixelsEachFrame * sizeof(unsigned short);
    int TotalCol = dCols * NDCMs;

    /*This sets the NSR and NPR in the leach assembly.*/
    pArcDev->SetImageSize(dRows, dCols);
    pArcDev->Command(TIM_ID, STC, TotalCol);




    //Buffer for 2x the frame size + 2 bytes to prevent overfill, but check where the boundary is
    size_t _setCommonBufferSize = dImageSizeEachFrame*2+2;
    pArcDev->ReMapCommonBuffer(_setCommonBufferSize);


    /*Check what has been set so far*/
    printf("Each frame: Rows %d, Cols %d | NDCMS: %d | Total columns: %d | Pixels: %d\n",
           pArcDev->GetImageRows(), pArcDev->GetImageCols(), this->CCDParams.nSkipperR, TotalCol, dExpectedPixelsEachFrame);



    /* Check for adequate buffer size */
    if (dImageSizeEachFrame > pArcDev->CommonBufferSize())
    {
        printf("Frame dimensions [ %d x %d ] exceed buffer size: %d.\n", dCols, dRows, pArcDev->CommonBufferSize());
        /*Guidance on how many NDCMs can be accomodated*/
        size_t BufferSize = pArcDev->CommonBufferSize();
        int MaxAllowableNDCMs = (BufferSize-2) / (dRows*dCols*sizeof(unsigned short));
        printf("Max allowable NDCMs for a single frame with dimensions [ %d x %d ] is %d\n",dRows, dCols, MaxAllowableNDCMs);

        /*Quit execution*/
        throw std::runtime_error("Image dimensions of each frame exceed the allowable buffer size.");
    }



    // Check for valid frame count
    if (dNumOfFrames <= 0)
        throw std::runtime_error("The number of frames must be > 0.");
    ChkAbortExposure;



    /* We have made it thus far, so we will calculate the frames per buffer
     * This will be 2 for very large images, and more otherwise.
     */
    dFramesPerBuffer = (int)pArcDev->CommonBufferSize() / dImageSizeEachFrame;


    /*Try to acquire the frames!*/
    try
    {
        // Skipper CCD related parameters
        if (this->CCDParams.CCDType == "SK")
            this->SetSSR();
        else
            this->CCDParams.nSkipperR = 1;


        /* Continuous readout parameter 1 : Frames per buffer -> SuperSequencer */
        dRetVal = pArcDev->Command(TIM_ID, FPB, dFramesPerBuffer);
        if (dRetVal != DON)
        {
            printf("Set FramePerBuffer command failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because FPB command failed.");
        }

        ChkAbortExposure;


        /* Continuous readout parameter 2 : Set the number of frames-to-take -> SuperSequencer
         * This is what the supersequencer uses to "know" that it is in the
         * continuous readout mode
         */
        dRetVal = pArcDev->Command(TIM_ID, SNF, dNumOfFrames);
        if (dRetVal != DON)
        {
            printf("Set SetNumberOfFrames command failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because SNF command failed.");
        }

        ChkAbortExposure;


        // Set the exposure time
        int dExpTime = (int)(fExpTime * 1000.0);
        dRetVal = pArcDev->Command(TIM_ID, SET, dExpTime);
        if (dRetVal != DON)
        {
            printf("Set exposure time failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because SET command failed.");
        }


        // Start the exposure
        dRetVal = pArcDev->Command(TIM_ID, SEX);
        if (dRetVal != DON)
        {
            printf("Start exposure command failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because SEX command failed.");
        }

        ChkAbortExposure;



        /* Iterate on all frames to read */
        while (dPCIFrameCount < dNumOfFrames)
        {
            ChkAbortExposure;

            //What is the current frame count?
            dPCIFrameCount = pArcDev->GetFrameCount();

            //{
            if (pArcDev->IsReadout())
            {
                bInReadout = true;
            }

            /* If we just moved from exposure to radout, then set the clocks
             * to reflect this
             */

            if (bLastInReadoutState != bInReadout && bLastInReadoutState==false)
            {

                /*Set the clock timers to readout mode*/
                if (this->ClockTimers.rClockCounter == 0)
                {
                    this->ClockTimers.Readoutstart = std::chrono::system_clock::now();
                    this->ClockTimers.isReadout = true;
                    this->ClockTimers.isExp = false;
                    this->ClockTimers.rClockCounter = 1;
                    this->ReadoutProgress.SetEssentials(dExpectedPixelsEachFrame,
                                                        this->ClockTimers.Readoutstart, dNumOfFrames);
                }
            }
            // ----------------------------
            // READ ELAPSED EXPOSURE TIME
            // ----------------------------
            // Checking the elapsed time > 1 sec. is to prevent race conditions with
            // sending RET while the PCI board is going into readout. Added check
            // for exposure_time > 1 sec. to prevent RET error.
            if (!bInReadout && fRemainingTime > 1.1f && dExposeCounter >= 5 &&
                    fExpTime > 1.0f)
            {
                // Ignore all RET timeouts
                try
                {
                    // Read the elapsed exposure time.
                    dRetVal = pArcDev->Command(TIM_ID, RET);

                    if (dRetVal != ROUT)
                    {
                        if (pArcDev->ContainsError(dRetVal) ||
                                pArcDev->ContainsError(dRetVal, 0, int(fExpTime * 1000)))
                        {
                            pArcDev->StopExposure();
                            throw std::runtime_error("Failed to read elapsed time!");
                        }

                        ChkAbortExposure;

                        dExposeCounter = 0;
                        fRemainingTime = fExpTime - (float)(dRetVal / 1000);

                        //if (pExpIFace != NULL) {
                        printf("\rExposure time remaining: %.3f ",fRemainingTime);

                        //}
                    }
                }
                catch (...)
                {
                }
            }

            dExposeCounter++;

            // ----------------------------
            // READOUT PIXEL COUNT
            // ----------------------------
            ChkAbortExposure;

            // Save the last pixel count for use by the timeout counter.
            dLastPixelCount = dPixelCount;
            dPixelCount = pArcDev->GetPixelCount();


            if (pArcDev->ContainsError(dPixelCount))
            {
                pArcDev->StopExposure();
                throw std::runtime_error("Failed to read pixel count!");
            }

            ChkAbortExposure;

            if (bInReadout)
            {
                //printf("\nPixels read: %d of %d ",dPixelCount, dExpectedPixelsEachFrame);
                pConIFace->ReadCallbackPixel(dPixelCount);
            }

            ChkAbortExposure;

            // If the controller's in READOUT, then increment the timeout
            // counter. Checking for readout prevents timeouts when clearing
            // large and/or slow arrays.
            if (bInReadout && dPixelCount == dLastPixelCount)
            {
                dTimeoutCounter++;
            }
            else
            {
                dTimeoutCounter = 0;
            }

            ChkAbortExposure;

            if (dTimeoutCounter >= pArcDev->READ_TIMEOUT)
            {
                pArcDev->StopExposure();
                throw std::runtime_error("Read timeout!");
            }



            //}


            // Read the images
            // while (dPCIFrameCount < dNumOfFrames) {


            //     ChkAbortExposure;



            Arc_Sleep(50);

            //}

            if (dFPBCount >= dFramesPerBuffer)
            {
                dFPBCount = 0;
            }

            dPCIFrameCount = pArcDev->GetFrameCount();
            if (dPCIFrameCount > dLastPCIFrameCount)
            {
                // Call external deinterlace and fits file functions here
                if (pConIFace != NULL)
                {
                    pConIFace->FrameCallback(dFPBCount, dPCIFrameCount, dRows, dCols,
                                             ((unsigned char*)pArcDev->CommonBufferVA()) +
                                             dFPBCount * dImageSizeEachFrame);
                }

                dLastPCIFrameCount = dPCIFrameCount;
                dFPBCount++;



            }

            /*Reset pixel counters*/
            dLastPixelCount = 0;
            dPixelCount = 0;
            bLastInReadoutState = bInReadout;


        }

        // Set back to single image mode
        dRetVal = pArcDev->Command(TIM_ID, SNF, 1);
        if (dRetVal != DON)
        {
            printf("Could not set device back to single image mode. Reply: 0x%X\n",
                   dRetVal);
            throw std::runtime_error(
                "Exception thrown because SNF 1 command failed.");
        }
    }
    catch (std::runtime_error& e)
    {
        // Set back to single image mode
        pArcDev->StopContinuous();

        throw e;
    }
}
