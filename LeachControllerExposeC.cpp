/* *********************************************************************
 * This file contains all the exposure related routines of the Leach CCD
 * control - CCDDrone.
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
#define MaxDataPerFrame 1200000
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
                                       std::string OutFileName, int numFrames)
{
    /* Strategy: Get the max memory possible or 1 GB whichever one is lower.
     * We will stick with 1 GB chunks or close to 1 GB to complete a row.
     */

    size_t RowsPerImageBlock;
    size_t NumContinuousReads;
    size_t ReminderRows;

    this->TotalPixelsToRead =
        this->CCDParams.dCols * this->CCDParams.dRows * this->CCDParams.nSkipperR;
    size_t ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows *
                             this->CCDParams.nSkipperR * sizeof(unsigned short);
    int TotalCol = this->CCDParams.dCols * this->CCDParams.nSkipperR;
    FitsOps FITSImage(OutFileName, this->CCDParams, this->BiasParams,
                      this->ClockParams, this->ClockTimers);

    /*Parameters needed for stat tracking*/
    this->TotalPixelsCounted = 0;
    this->ReadoutProgress.SetEssentials(this->TotalPixelsToRead,
                                        this->ClockTimers.Readoutstart);


    /*Keep the old method for full exposures,
     * and so we have a basis of comparison if needed
     */



//        this->TotalChunks = 1;
//        this->CurrentChunk = 1;
//        this->ReadoutProgress.updProgressPart(this->CurrentChunk,
//                                              this->TotalChunks);
//        unsigned short* ImageBufferV;
//        this->PrepareAndExposeCCD(ExposeSeconds, &ImageBufferV);
//
//        /*Write the data*/
//        FITSImage.WriteData(0, 0, this->CCDParams.dRows,
//                            this->CCDParams.dCols * this->CCDParams.nSkipperR,
//                            ImageBufferV);
//
//        /*Write the post exposure info*/
//        FITSImage.WritePostExposureInfo();


    /* ******************************************
     * Expose strategy 3 - continuous exposure
     *
     * In this mode, the Leach system itself performs
     * a continuous readout of many frames and stores
     * these frames in a ringbuffer like situation.
     * *******************************************/


    /*Size of each row*/
    size_t SingleRow = this->CCDParams.dCols * this->CCDParams.nSkipperR *
                       sizeof(unsigned short);
    /*Size of each block */
    size_t BlockSize = SingleRow * NumRowsInContReadout;
    /*How many blocks do we need to fill the entire image?*/
    NumContinuousReads = numFrames;

    /*Debugging info*/
    std::string ExposeStrategyTxt = "CCDDExposeC (continuous readout)";
    ExposeStrategyTxt = ColouredFmtText(ExposeStrategyTxt, "magenta");
    std::string ExposeStrategyExtra =
        std::to_string(NumContinuousReads) + " parts.";
    ExposeStrategyExtra = ColouredFmtText(ExposeStrategyExtra, "red", "bold");
    std::cout << ExposeStrategyTxt << " | The image will be read in "
              << ExposeStrategyExtra << "\n";

    this->TotalChunks = NumContinuousReads;

    /*Set up the FITS file header*/
    FITSImage.WriteHeader();


    /*Perform readout*/
    //this->_FitsFile = &FITSImage; //This is for cConIface to modify the fits file.
    CMyConIFace cMyConIFace(*this, NumRowsInContReadout, NumContinuousReads, &FITSImage);



    this->ExposeContinuous(NumRowsInContReadout, this->CCDParams.dCols, this->CCDParams.nSkipperR,
                           NumContinuousReads, this->CCDParams.fExpTime,
                           false,
                           &cMyConIFace,
                           false);

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
// |  <IN> -> bOpenShutter - 'true' to open the shutter during expose; 'false'
// |                         otherwise.
// +----------------------------------------------------------------------------
void LeachController::ExposeContinuous(int dRows, int dCols, int NDCMs,
                                       int dNumOfFrames, float fExpTime,
                                       const bool& bAbort,
                                       CMyConIFace::CConIFace* pConIFace,
                                       bool bOpenShutter)
{
    int dFramesPerBuffer = 0;
    int dPCIFrameCount = 0;
    int dLastPCIFrameCount = 0;
    int dFPBCount = 0;

    float fRemainingTime = fExpTime;
    bool bInReadout = false;
    int dTimeoutCounter = 0;
    int dLastPixelCount = 0;
    int dPixelCount = 0;
    int dExposeCounter = 0;

    int dRetVal;

    /*Number of pixels to read*/
    size_t dExpectedPixelsEachFrame = dRows * dCols * NDCMs;
    size_t dImageSizeEachFrame = dExpectedPixelsEachFrame * sizeof(unsigned short);

    int TotalCol = dCols * NDCMs;
    /*This sets the NSR and NPR in the leach assembly.*/
    pArcDev->SetImageSize(dRows, dCols);
    pArcDev->Command(TIM_ID, STC, TotalCol);


    //Buffer for 2x the frame size
    size_t _setCommonBufferSize = dImageSizeEachFrame*2+2;
    pArcDev->ReMapCommonBuffer(_setCommonBufferSize);

    printf("Rows %d, Cols %d | NDCMS: %d , Total number of columns: %d\n",
           pArcDev->GetImageRows(), pArcDev->GetImageCols(),
           this->CCDParams.nSkipperR, TotalCol);


    // Check for adequate buffer size
    if (dImageSizeEachFrame > pArcDev->CommonBufferSize())
    {
        printf("Image dimensions [ %d x %d ] exceed buffer size: %d.\n", dCols,
               dRows, pArcDev->CommonBufferSize());
        throw std::runtime_error("Image dimensions exceed buffer size.");
    }

    // Check for valid frame count
    if (dNumOfFrames <= 0)
        throw std::runtime_error("The number of frames is less than 0.");

    ChkAbortExposure;

    // int/size_t will get promoted to size_t since its unsigned.
    printf ("Checkpoint 1\n");
    dFramesPerBuffer = (int)pArcDev->CommonBufferSize() / dImageSizeEachFrame;

    printf("CommonBufferSize: %d\n",pArcDev->CommonBufferSize());
    printf("dImageSizeEachFrame: %d\n",dImageSizeEachFrame);
    printf("Frames per buffer: %d\n",dFramesPerBuffer);
    printf("DNumofFrames: %d\n",dNumOfFrames);

    ChkAbortExposure;


    printf ("Checkpoint 2\n");

    try
    {
        // Skipper CCD related parameters
        if (this->CCDParams.CCDType == "SK")
            this->SetSSR();
        else
            this->CCDParams.nSkipperR = 1;

        printf ("Checkpoint 3\n");


        // Set the frames-per-buffer
        dRetVal = pArcDev->Command(TIM_ID, FPB, dFramesPerBuffer);

        if (dRetVal != DON)
        {
            printf("Set FramePerBuffer command failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because FPB command failed.");
        }

        ChkAbortExposure;

        printf ("Checkpoint 4\n");

        // Set the number of frames-to-take
        dRetVal = pArcDev->Command(TIM_ID, SNF, dNumOfFrames);
        if (dRetVal != DON)
        {
            printf("Set SetNumberOfFrames command failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because SNF command failed.");
        }

        ChkAbortExposure;

        printf ("Checkpoint 5\n");

        // Set the shutter position
        // SetOpenShutter( bOpenShutter );

        // Set the exposure time
        int dExpTime = (int)(fExpTime * 1000.0);
        dRetVal = pArcDev->Command(TIM_ID, SET, dExpTime);

        if (dRetVal != DON)
        {
            printf("Set exposure time failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because SET command failed.");
        }

        //
        // Start the exposure
        //
        dRetVal = pArcDev->Command(TIM_ID, SEX);

        if (dRetVal != DON)
        {
            printf("Set exposure command failed. Reply: 0x%X\n", dRetVal);
            throw std::runtime_error("Exception thrown because SEX command failed.");
        }

        ChkAbortExposure;

        printf ("Checkpoint 6\n");

        while (dPCIFrameCount < dNumOfFrames)
        {

            ChkAbortExposure;
            dPCIFrameCount = pArcDev->GetFrameCount();
            printf("\nPCI FrameCount: %d, Expected %d",dPCIFrameCount,dNumOfFrames);
            printf("InReadout %d\n",pArcDev->IsReadout());

            //while (dPixelCount < dExpectedPixelsEachFrame)
            //{
            printf ("Checkpoint 7\n");
            if (pArcDev->IsReadout()) {
                bInReadout = true;
            }
//
//                /*Set the clock timers to readout mode*/
//                if (this->ClockTimers.rClockCounter == 0) {
//                    this->ClockTimers.Readoutstart = std::chrono::system_clock::now();
//                    this->ClockTimers.isReadout = true;
//                    this->ClockTimers.isExp = false;
//                    this->ClockTimers.rClockCounter = 1;
//                    this->ReadoutProgress.SetEssentials(this->TotalPixelsToRead,
//                                                        this->ClockTimers.Readoutstart);
//                    std::cout << std::endl
//                            << "Total pixels to read: " << this->TotalPixelsToRead
//                            << std::endl;
//                }
//                // printf("Is in readout: %d\n",pArcDev->IsReadout());
//            }

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
                printf("\nPixels read: %d of %d ",dPixelCount, dExpectedPixelsEachFrame);

                if (pArcDev->ContainsError(dPixelCount))
                {
                    pArcDev->StopExposure();
                    throw std::runtime_error("Failed to read pixel count!");
                }

                ChkAbortExposure;

                if (bInReadout)
                {
                    printf("\nPixels read: %d of %d ",dPixelCount, dExpectedPixelsEachFrame);
                    //pExpIFace->ReadCallback(dPixelCount);
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
                //         // Call external deinterlace and fits file functions here
                //         if (pConIFace != NULL) {
                //             pConIFace->FrameCallback(dFPBCount, dPCIFrameCount, dRows, dCols,
                //                                      ((unsigned char*)pArcDev->CommonBufferVA()) +
                //                                      dFPBCount * dBoundedImageSize);
                //         }

                dLastPCIFrameCount = dPCIFrameCount;
                dFPBCount++;



            }

            /*Reset pixel counters*/
            dLastPixelCount = 0;
            dPixelCount = 0;


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
