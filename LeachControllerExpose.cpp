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
#define MaxDataPerFrame 536870912
#define NumRowsInContReadout 20

#define OverrideExposeStrategy 0
#define OverrideStrategyNumber 2

/* This "abort exposure" condition seems to get executed a lot
 * so its better to use the preprocessor to reduce clutter
 */

#define ChkAbortExposure                         \
  if (bAbort) {                                  \
    pArcDev->StopExposure();                     \
    throw std::runtime_error("Expose Aborted!"); \
  }

int LeachController::DecideStrategyAndExpose(int ExposeSeconds,
                                             std::string OutFileName) {
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

  /*Decide strategy here
   *Strategy
   *0 - Not calculated yet
   *1 - Single image, old method
   *2 - Piecewise read and join
   *3 - Continuous read and join
   */
  int ExposeStrategy = 0;

  if (OverrideExposeStrategy)
    ExposeStrategy = OverrideStrategyNumber;
  else {
    if (ImageMemorySize < MaxDataPerFrame)
      ExposeStrategy = 1;
    else
      ExposeStrategy = 3;
  }

  /*Keep the old method for full exposures,
   * and so we have a basis of comparison if needed
   */

  if (ExposeStrategy == 1) {
    /*Debugging info*/
    std::string ExposeStrategy = "Strategy: 1 (single readout)";
    ExposeStrategy = ColouredFmtText(ExposeStrategy, "magenta");
    ExposeStrategy += " | The image will be read all at once.\n";
    std::cout << ExposeStrategy;

    /*Set up the FITS file header*/
    FITSImage.WriteHeader();

    this->TotalChunks = 1;
    this->CurrentChunk = 1;
    this->ReadoutProgress.updProgressPart(this->CurrentChunk,
                                          this->TotalChunks);
    unsigned short* ImageBufferV;
    this->PrepareAndExposeCCD(ExposeSeconds, &ImageBufferV);

    /*Write the data*/
    FITSImage.WriteData(0, 0, this->CCDParams.dRows,
                        this->CCDParams.dCols * this->CCDParams.nSkipperR,
                        ImageBufferV);

    /*Write the post exposure info*/
    FITSImage.WritePostExposureInfo();

  } else if (ExposeStrategy == 2) {
    /*Size of each row*/
    size_t SingleRow = this->CCDParams.dCols * this->CCDParams.nSkipperR *
                       sizeof(unsigned short);
    /*How many rows in 1GB ish*/
    RowsPerImageBlock = floor(MaxDataPerFrame / SingleRow);
    /*Imagine each "block" of row*RowsPerImageBlock. How many do we need to fill
     * the entire image?*/
    NumContinuousReads = floor(this->CCDParams.dRows / RowsPerImageBlock);
    /*How many rows will remain after this?*/
    ReminderRows = this->CCDParams.dRows % RowsPerImageBlock;

    /*Debugging info*/
    std::string ExposeStrategy = "Strategy: 2 (segmented readout)";
    ExposeStrategy = ColouredFmtText(ExposeStrategy, "magenta");
    std::string ExposeStrategyExtra =
        std::to_string(NumContinuousReads + 1) + " parts.";
    ExposeStrategyExtra = ColouredFmtText(ExposeStrategyExtra, "red", "bold");
    std::cout << ExposeStrategy << " | The image will be read in "
              << ExposeStrategyExtra << "\n";

    this->TotalChunks = NumContinuousReads + 1;

    /*Set up the FITS file header*/
    FITSImage.WriteHeader();

    /*For loop over all the blocks of data*/
    for (int i = 0; i < NumContinuousReads; i++) {
      this->CurrentChunk = i + 1;
      this->ReadoutProgress.updProgressPart(this->CurrentChunk,
                                            this->TotalChunks);

      unsigned short* ImageBufferV;
      if (i == 0) {  // First read
        this->PrepareAndExposeCCDForLargeImages(
            ExposeSeconds, RowsPerImageBlock, &ImageBufferV, true, false);
        FITSImage.WriteData(i * RowsPerImageBlock, 0, RowsPerImageBlock,
                            this->CCDParams.dCols * this->CCDParams.nSkipperR,
                            ImageBufferV);

      } else {  // all other reads
        this->PrepareAndExposeCCDForLargeImages(
            ExposeSeconds, RowsPerImageBlock, &ImageBufferV, false, false);
        FITSImage.WriteData(i * RowsPerImageBlock, 0, RowsPerImageBlock,
                            this->CCDParams.dCols * this->CCDParams.nSkipperR,
                            ImageBufferV);
      }
    }

    /*Last block of reads - in its own scope*/
    {
      this->CurrentChunk = NumContinuousReads + 1;
      this->ReadoutProgress.updProgressPart(this->CurrentChunk,
                                            this->TotalChunks);

      unsigned short* ImageBufferV;
      this->PrepareAndExposeCCDForLargeImages(ExposeSeconds, ReminderRows,
                                              &ImageBufferV, false, true);
      FITSImage.WriteData(
          NumContinuousReads * RowsPerImageBlock, 0, ReminderRows,
          this->CCDParams.dCols * this->CCDParams.nSkipperR, ImageBufferV);
    }

    /*Write the post exposure info*/
    FITSImage.WritePostExposureInfo();

    /*reset image dimensions to block size*/
    pArcDev->SetImageSize(RowsPerImageBlock, this->CCDParams.dCols);


    /* ******************************************
     * Expose strategy 3 - continuous exposure
     *
     * In this mode, the Leach system itself performs
     * a continuous readout of many frames and stores
     * these frames in a ringbuffer like situation.
     * *******************************************/

    } else if (ExposeStrategy == 3) {
    /*Size of each row*/
    size_t SingleRow = this->CCDParams.dCols * this->CCDParams.nSkipperR *
                       sizeof(unsigned short);
    /*Size of each block */
    size_t BlockSize = SingleRow * NumRowsInContReadout;
    /*How many blocks do we need to fill the entire image?*/
    NumContinuousReads = 1 + this->CCDParams.dRows / (int)RowsPerImageBlock;
    /*How many rows will remain after this?*/
    ReminderRows =
        (NumContinuousReads * (int)RowsPerImageBlock) - this->CCDParams.dRows;

    /*Debugging info*/
    std::string ExposeStrategy = "Strategy: 3 (continuous readout)";
    ExposeStrategy = ColouredFmtText(ExposeStrategy, "magenta");
    std::string ExposeStrategyExtra =
        std::to_string(NumContinuousReads) + " parts.";
    ExposeStrategyExtra = ColouredFmtText(ExposeStrategyExtra, "red", "bold");
    std::cout << ExposeStrategy << " | The image will be read in "
              << ExposeStrategyExtra << "\n";

    this->TotalChunks = NumContinuousReads;

    /*Set up the FITS file header*/
    FITSImage.WriteHeader();


    /*Perform readout*/
    this->_FitsOps = &FITSImage; //This is for cConIface to modify the fits file.
    CMyConIFace cMyConIFace(*this, NumRowsInContReadout, NumContinuousReads);
    bool bAbort = false;

    this->ExposeContinuous(NumRowsInContReadout, this->CCDParams.dCols, this->CCDParams.nSkipperR,
                                       NumContinuousReads, this->CCDParams.fExpTime,
                                       &bAbort,
                                       &cMyConIFace,
                                       false);

    /*Write the post exposure info*/
    FITSImage.WritePostExposureInfo();
  }

  return 0;
}

/* *********************************************************************
 * This is the CCD exposure preparation routine. This routine should be
 * launched by programs wishing to do an exposure. This will ensure that
 * appropreate parameters (nSkipperR for SK CCDs) etc are set properly
 * before an exposure is taken.
 * This routine will also set SSR values before a skipper exposure.
 * *********************************************************************
 */

void LeachController::PrepareAndExposeCCD(int ExposureTime,
                                          unsigned short** ImageBuffer) {
  try {
    if (this->CCDParams.CCDType == "SK")
      this->SetSSR();
    else
      this->CCDParams.nSkipperR = 1;

    /*Needed for callbacks during exposure*/
    CExposeListener cExposeListener(*this);

    size_t ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows *
                             this->CCDParams.nSkipperR * sizeof(unsigned short);
    int TotalCol = this->CCDParams.dCols * this->CCDParams.nSkipperR;

    /*This sets the NSR and NPR in the leach assembly.*/
    pArcDev->SetImageSize(this->CCDParams.dRows, this->CCDParams.dCols);
    pArcDev->Command(TIM_ID, STC, TotalCol);

    pArcDev->ReMapCommonBuffer(ImageMemorySize);
    printf("Rows %d, Cols %d | NDCMS: %d , Total number of columns: %d\n",
           pArcDev->GetImageRows(), pArcDev->GetImageCols(),
           this->CCDParams.nSkipperR, TotalCol);

    /*This will happen if the memory required is > kernel buffer size*/
    if (pArcDev->CommonBufferSize() < ImageMemorySize) {
      std::cout << "Common buffer size: " << pArcDev->CommonBufferSize()
                << "  | Image memory requirement: " << ImageMemorySize << "\n";
      throw std::runtime_error("Failed to map image buffer!");
    }

    /*De-Interlacing part*/
    int dDeintAlg;
    if (this->CCDParams.AmplifierDirection == "UL") {
      dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_SERIAL;
    } else if (this->CCDParams.AmplifierDirection == "U" ||
               this->CCDParams.AmplifierDirection == "L") {
      dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
    } else {
      std::cout << "The amplifier selected does not exist. Interlacing is not "
                   "set. Stop and verify!\n";
    }

    /*To eliminate the possibility of the amplifiers behaving as a source of
     * light, we turn off the Vdd to them before the exposure starts. We then
     * turn them back on 3 seconds before the exposure ends
     */

    if (ExposureTime > 3) {
      std::cout << "Turning VDD OFF before exposure.\n";
      this->ToggleVDD(0);
    } else {
      std::cout << "Exposure time is too short to turn VDD off.\n";
    }
    std::cout << "Starting exposure\n";
    this->ExposeCCD(ExposureTime, false, &cExposeListener);
    this->ClockTimers.ReadoutEnd = std::chrono::system_clock::now();
    this->ReadoutProgress.done();
    std::cout << "\nExposure complete.\n";

    /*If two amplifiers were used, we need to de-interlace*/
    if (this->CCDParams.AmplifierDirection == "UL" ||
        this->CCDParams.AmplifierDirection == "LU") {
      std::cout << "Since amplifier selected was UL / LU, the image will now "
                   "be de-interlaced.\n";
      unsigned short* pU16Buf = (unsigned short*)pArcDev->CommonBufferVA();
      arc::deinterlace::CArcDeinterlace cDlacer;
      cDlacer.RunAlg(pU16Buf, this->CCDParams.dRows,
                     this->CCDParams.dCols * this->CCDParams.nSkipperR,
                     dDeintAlg);
    }

    /*Set the imageBuffer before finishing*/
    *ImageBuffer = (unsigned short*)this->pArcDev->CommonBufferVA();

    /*Calculate and store the clock durations*/
    auto ExpDuration = std::chrono::duration<double, std::milli>(
        this->ClockTimers.Readoutstart - this->ClockTimers.ExpStart);
    auto RdoutDuration = std::chrono::duration<double, std::milli>(
        this->ClockTimers.ReadoutEnd - this->ClockTimers.Readoutstart);
    this->ClockTimers.MeasuredExp = ExpDuration.count();
    this->ClockTimers.MeasuredReadout = RdoutDuration.count();

    /* In case we run into a runtime error */
  } catch (std::runtime_error& e) {
    std::cout << "failed!" << std::endl;
    std::cerr << std::endl << e.what() << std::endl;

    if (pArcDev->IsReadout()) {
      pArcDev->StopExposure();
    }

    /* Or any other kind of error */
  } catch (...) {
    std::cerr << std::endl
              << "Error: unknown exception occurred!!!" << std::endl;

    if (pArcDev->IsReadout()) {
      pArcDev->StopExposure();
    }
  }
}

/* *********************************************************************
 * This is the CCD exposure routine. This is common to both Skipper
 * and DES. In case of DES, it counts on nSkipperR being = 1.
 *
 * Input:
 * fExpTime - The exposure time ( in seconds ).
 * bOpenShutter - Set to 'true' if the shutter should open during the
 *               exposure. Set to 'false' to keep the shutter closed.
 * fExpTime - The exposure time ( in seconds ).
 * bAbort - Pointer to boolean value that can cause the readout
 *                   method to abort/stop either exposing or image readout.
 *                   NULL by default.
 * pExpIFace - Function pointer to CExpIFace class. NULL by default.
 * *********************************************************************
 */

void LeachController::ExposeCCD(float fExpTime, const bool& bAbort,
                                CExposeListener::CExpIFace* pExpIFace,
                                bool bOpenShutter) {
  float fRemainingTime = fExpTime;
  bool bInReadout = false;
  int dTimeoutCounter = 0;
  int dLastPixelCount = 0;
  int dPixelCount = 0;
  int dExposeCounter = 0;

  /*Number of pixels to read*/
  this->TotalPixelsToRead =
      this->CCDParams.dCols * this->CCDParams.dRows * this->CCDParams.nSkipperR;

  /* Check for adequate buffer size */
  size_t ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows *
                           this->CCDParams.nSkipperR * sizeof(unsigned short);

  if (ImageMemorySize > pArcDev->CommonBufferSize()) {
    printf("Image dimensions [ %d x %d x %d ] exceed buffer size: %d.",
           this->CCDParams.dCols, this->CCDParams.dRows,
           this->CCDParams.nSkipperR, pArcDev->CommonBufferSize());
    throw std::runtime_error(
        "Exception thrown because buffer size is too small for the image.");
  }

  /* Set the shutter position */
  pArcDev->SetOpenShutter(bOpenShutter);

  /* Set the exposure time */
  int dRetVal = pArcDev->Command(TIM_ID, SET, int(fExpTime * 1000.0));

  if (dRetVal != DON) {
    printf("Set exposure time failed. Reply: 0x%X\n", dRetVal);
    throw std::runtime_error("Exception thrown because SET command failed.");
  }

  /* Start the exposure */
  this->ClockTimers.ExpStart = std::chrono::system_clock::now();
  this->ClockTimers.isExp = true;
  dRetVal = pArcDev->Command(TIM_ID, SEX);
  if (dRetVal != DON) {
    printf("Start exposure command failed. Reply: 0x%X\n", dRetVal);
    throw std::runtime_error("Exception thrown because SEX command failed.");
  }

  while (dPixelCount < (this->CCDParams.dRows * this->CCDParams.dCols *
                        this->CCDParams.nSkipperR)) {
    if (pArcDev->IsReadout()) {
      bInReadout = true;

      /*Set the clock timers to readout mode*/
      if (this->ClockTimers.rClockCounter == 0) {
        this->ClockTimers.Readoutstart = std::chrono::system_clock::now();
        this->ClockTimers.isReadout = true;
        this->ClockTimers.isExp = false;
        this->ClockTimers.rClockCounter = 1;
        this->ReadoutProgress.SetEssentials(this->TotalPixelsToRead,
                                            this->ClockTimers.Readoutstart);
        std::cout << std::endl
                  << "Total pixels to read: " << this->TotalPixelsToRead
                  << std::endl;
      }
      // printf("Is in readout: %d\n",pArcDev->IsReadout());
    }

    // ----------------------------
    // READ ELAPSED EXPOSURE TIME
    // ----------------------------
    // Checking the elapsed time > 1 sec. is to prevent race conditions with
    // sending RET while the PCI board is going into readout. Added check
    // for exposure_time > 1 sec. to prevent RET error.
    if (!bInReadout && fRemainingTime > 1.1f && dExposeCounter >= 5 &&
        fExpTime > 1.0f) {
      // Ignore all RET timeouts
      try {
        // Read the elapsed exposure time.
        dRetVal = pArcDev->Command(TIM_ID, RET);

        if (dRetVal != ROUT) {
          if (pArcDev->ContainsError(dRetVal) ||
              pArcDev->ContainsError(dRetVal, 0, int(fExpTime * 1000))) {
            pArcDev->StopExposure();
            throw std::runtime_error("Failed to read elapsed time!");
          }

          ChkAbortExposure;

          dExposeCounter = 0;
          fRemainingTime = fExpTime - (float)(dRetVal / 1000);

          if (pExpIFace != NULL) {
            pExpIFace->ExposeCallback(fRemainingTime);
          }
        }
      } catch (...) {
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

    if (pArcDev->ContainsError(dPixelCount)) {
      pArcDev->StopExposure();
      throw std::runtime_error("Failed to read pixel count!");
    }

    ChkAbortExposure;

    if (bInReadout && pExpIFace != NULL) {
      pExpIFace->ReadCallback(dPixelCount);
    }

    ChkAbortExposure;

    // If the controller's in READOUT, then increment the timeout
    // counter. Checking for readout prevents timeouts when clearing
    // large and/or slow arrays.
    if (bInReadout && dPixelCount == dLastPixelCount) {
      dTimeoutCounter++;
    } else {
      dTimeoutCounter = 0;
    }

    ChkAbortExposure;

    if (dTimeoutCounter >= pArcDev->READ_TIMEOUT) {
      pArcDev->StopExposure();
      throw std::runtime_error("Read timeout!");
    }

    Arc_Sleep(50);
  }
}

/* *********************************************************************
 * This is the CCD exposure preparation routine for large images.
 * This routine should be launched when the image to be taken is larger
 * than what is reasonable for a driver to occupy in the lower 4GB of
 * your RAM. I have set this limit to 1 GB for now.
 *
 * This routine will:
 * 1. Stop the IDLE clocking and set NDCMs / SSR.
 * 2. Set the sub-image dimensions for all the exposures
 * 3. Perform an expose if it is the first in the sequence of reads.
 * 4. Perform a 0 second exposure for subsequent reads (the Leach
 *     (assembly code is programmed to bypass shutter and waits if
 *     the exposure time is set to 0)
 * 5. Re-set the image dimensions for the last exposure and perform it.
 * 6. Set the image dimensions to accomodate reasonable 1GB
 *     sub image before the code finishes.
 * *********************************************************************
 */

void LeachController::PrepareAndExposeCCDForLargeImages(
    int ExposureTime, int dRows, unsigned short** ImageBuffer,
    bool FirstInSequence, bool LastInSequence) {
  /*Needed for callbacks during exposure*/
  CExposeListener cExposeListener(*this);

  /*Variables needed to store status information*/
  int dDeintAlg, dRetVal;
  int TotalCol = this->CCDParams.dCols * this->CCDParams.nSkipperR;

  try {
    /* If it is the first exposure, we will have to:
     * 1. SetSSR for the correct number of NDCMs.
     * 2. Turn OFF IDLE clocks.
     * 3. Set the correct exposure time. NOTE: We have to change this after the
     * first exposure.
     * 4. Set Deinterlacing.
     */
    if (FirstInSequence) {
      if (this->CCDParams.CCDType == "SK")
        this->SetSSR();
      else
        this->CCDParams.nSkipperR = 1;

      /*We should set the IDLE clocking OFF here*/
      /*Next in sequence is setting the exposure time*/
      dRetVal = pArcDev->Command(TIM_ID, STP);
      if (dRetVal != DON) {
        printf("Stop clock failed. Reply: 0x%X\n", dRetVal);
        throw std::runtime_error(
            "Exception thrown because HLD command failed.");
      }

      /*To eliminate the possibility of the amplifiers behaving as a source of
       * light, we turn off the Vdd to them before the exposure starts. We then
       * turn them back on 3 seconds before the exposure ends
       */
      if (ExposureTime > 3) {
        // std::cout << "Turning VDD OFF before exposure.\n";
        this->ToggleVDD(0);
      } else {
        // std::cout << "Exposure time is too short to turn VDD off.\n";
      }

      /*Next is the shutter settings. We have no idea what it does*/
      bool bOpenShutter = false;
      pArcDev->SetOpenShutter(bOpenShutter);

      /*Next in sequence is setting the exposure time*/
      dRetVal = pArcDev->Command(TIM_ID, SET, int(ExposureTime * 1000.0));
      if (dRetVal != DON) {
        printf("Set exposure time failed. Reply: 0x%X\n", dRetVal);
        throw std::runtime_error(
            "Exception thrown because SET command failed.");
      }

      /*De-Interlacing part*/
      if (this->CCDParams.AmplifierDirection == "UL") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_SERIAL;
      } else if (this->CCDParams.AmplifierDirection == "U" ||
                 this->CCDParams.AmplifierDirection == "L") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
      } else {
        std::cout << "The amplifier selected does not exist. Interlacing is "
                     "not set. Stop and verify!\n";
      }
    }

    if (FirstInSequence || LastInSequence) {
      /*Expected image memory size is the size of a "block" - we set that here*/
      size_t ImageMemorySize = this->CCDParams.dCols * dRows *
                               this->CCDParams.nSkipperR *
                               sizeof(unsigned short);

      /*This sets the NSR and NPR in the leach assembly.*/
      pArcDev->SetImageSize(dRows, this->CCDParams.dCols);
      pArcDev->Command(TIM_ID, STC, TotalCol);

      pArcDev->ReMapCommonBuffer(ImageMemorySize);
      // printf("(ASM) This block: Rows %d, Cols %d | NDCMS: %d , Total number
      // of columns: %d\n", pArcDev->GetImageRows(),
      //       pArcDev->GetImageCols(), this->CCDParams.nSkipperR, TotalCol);

      /*This will happen if the memory required is > kernel buffer size*/
      if (pArcDev->CommonBufferSize() < ImageMemorySize) {
        std::cout << "Common buffer size: " << pArcDev->CommonBufferSize()
                  << "  | Image memory requirement: " << ImageMemorySize
                  << "\n";
        throw std::runtime_error("Failed to map image buffer!");
      }
    }

    if (!FirstInSequence && !LastInSequence) {
      printf(
          "(ASM) This block: Rows %d, Cols %d | NDCMS: %d , Total number of "
          "columns: %d\n",
          pArcDev->GetImageRows(), pArcDev->GetImageCols(),
          this->CCDParams.nSkipperR, TotalCol);
    }

    /*Needed for callbacks during exposure*/
    CExposeListener cExposeListener(*this);
    this->ExposeCCDChunk(ExposureTime, dRows, FirstInSequence, false,
                         &cExposeListener);

    /*After this part of the expose / readout is complete*/
    this->ClockTimers.ReadoutEnd = std::chrono::system_clock::now();
    this->ReadoutProgress.done();

    /*If two amplifiers were used, we need to de-interlace*/
    if (this->CCDParams.AmplifierDirection == "UL" ||
        this->CCDParams.AmplifierDirection == "LU") {
      std::cout << "Since amplifier selected was UL / LU, the image will now "
                   "be de-interlaced.\n";
      unsigned short* pU16Buf = (unsigned short*)pArcDev->CommonBufferVA();
      arc::deinterlace::CArcDeinterlace cDlacer;
      cDlacer.RunAlg(pU16Buf, dRows,
                     this->CCDParams.dCols * this->CCDParams.nSkipperR,
                     dDeintAlg);
    }

    /*Set the imageBuffer before finishing*/
    *ImageBuffer = (unsigned short*)this->pArcDev->CommonBufferVA();

    /* Exit Guard: First read
     * If this was the first read, then it is expected that the next reads
     * onwards will use 0 second exposures*/

    if (FirstInSequence) {
      /*Set the exposure time to 0 for the next read*/
      dRetVal = pArcDev->Command(TIM_ID, SET, int(0));

      if (dRetVal != DON) {
        printf("Set exposure time failed. Reply: 0x%X\n", dRetVal);
        throw std::runtime_error(
            "Exception thrown because SET command failed.");
      }
    }

    /* Exit guard - last read
     * If this was the last read, we set the exposure time to normal again*/
    if (LastInSequence) {
      int dRetVal = pArcDev->Command(TIM_ID, SET, int(ExposureTime * 1000.0));

      if (dRetVal != DON) {
        printf("Set exposure time failed. Reply: 0x%X\n", dRetVal);
        throw std::runtime_error(
            "Exception thrown because SET command failed.");
      }

      dRetVal = pArcDev->Command(TIM_ID, IDL);
      if (dRetVal != DON) {
        printf("Set IDLE failed after last exposure. Reply: 0x%X\n", dRetVal);
        throw std::runtime_error(
            "Exception thrown because IDL command failed.");
      }

      /*Calculate and store the clock durations*/
      auto ExpDuration = std::chrono::duration<double, std::milli>(
          this->ClockTimers.Readoutstart - this->ClockTimers.ExpStart);
      auto RdoutDuration = std::chrono::duration<double, std::milli>(
          this->ClockTimers.ReadoutEnd - this->ClockTimers.Readoutstart);
      this->ClockTimers.MeasuredExp = ExpDuration.count();
      this->ClockTimers.MeasuredReadout = RdoutDuration.count();
    }

    /* In case we run into a runtime error */
  } catch (std::runtime_error& e) {
    std::cout << "failed!" << std::endl;
    std::cerr << std::endl << e.what() << std::endl;

    if (pArcDev->IsReadout()) {
      pArcDev->StopExposure();
    }

    /* Or any other kind of error */
  } catch (...) {
    std::cerr << std::endl
              << "Error: unknown exception occurred!!!" << std::endl;

    if (pArcDev->IsReadout()) {
      pArcDev->StopExposure();
    }
  }
}

/* *********************************************************************
 * This is the CCD exposure routine for a big image reading a chunk
 *
 * Input:
 * fExpTime - The exposure time ( in seconds ).
 * bOpenShutter - Set to 'true' if the shutter should open during the
 *               exposure. Set to 'false' to keep the shutter closed.
 * fExpTime - The exposure time ( in seconds ).
 * bAbort - Pointer to boolean value that can cause the readout
 *                   method to abort/stop either exposing or image readout.
 *                   NULL by default.
 * pExpIFace - Function pointer to CExpIFace class. NULL by default.
 * *********************************************************************
 */

void LeachController::ExposeCCDChunk(float fExpTime, int dRows, bool FirstRead,
                                     const bool& bAbort,
                                     CExposeListener::CExpIFace* pExpIFace) {
  float fRemainingTime = fExpTime;
  bool bInReadout = false;
  int dTimeoutCounter = 0;
  int dLastPixelCount = 0;
  int dPixelCount = 0;
  int dExposeCounter = 0;
  int dRetVal;

  /* Start the exposure */
  this->ClockTimers.ExpStart = std::chrono::system_clock::now();
  this->ClockTimers.isExp = true;
  dRetVal = pArcDev->Command(TIM_ID, SEX);
  if (dRetVal != DON) {
    printf("Start exposure command failed. Reply: 0x%X\n", dRetVal);
    throw std::runtime_error("Exception thrown because SEX command failed.");
  }

  while (dPixelCount <
         (dRows * this->CCDParams.dCols * this->CCDParams.nSkipperR)) {
    if (pArcDev->IsReadout()) {
      bInReadout = true;

      /*Set the clock timers to readout mode*/
      if (this->ClockTimers.rClockCounter == 0) {
        this->ClockTimers.Readoutstart = std::chrono::system_clock::now();
        this->ClockTimers.isReadout = true;
        this->ClockTimers.isExp = false;
        this->ClockTimers.rClockCounter = 1;
        std::cout << std::endl
                  << "Total pixels to read: " << this->TotalPixelsToRead
                  << std::endl;
        /*Set progressbar*/
        if (FirstRead)
          this->ReadoutProgress.SetEssentials(this->TotalPixelsToRead,
                                              this->ClockTimers.Readoutstart);
      }
      // printf("Is in readout: %d\n",pArcDev->IsReadout());
    }

    // ----------------------------
    // READ ELAPSED EXPOSURE TIME
    // ----------------------------
    // Checking the elapsed time > 1 sec. is to prevent race conditions with
    // sending RET while the PCI board is going into readout. Added check
    // for exposure_time > 1 sec. to prevent RET error.
    if (!bInReadout && fRemainingTime > 1.1f && dExposeCounter >= 5 &&
        fExpTime > 1.0f) {
      // Ignore all RET timeouts
      try {
        // Read the elapsed exposure time.
        dRetVal = pArcDev->Command(TIM_ID, RET);

        if (dRetVal != ROUT) {
          if (pArcDev->ContainsError(dRetVal) ||
              pArcDev->ContainsError(dRetVal, 0, int(fExpTime * 1000))) {
            pArcDev->StopExposure();
            throw std::runtime_error("Failed to read elapsed time!");
          }

          ChkAbortExposure;

          dExposeCounter = 0;
          fRemainingTime = fExpTime - (float)(dRetVal / 1000);

          if (pExpIFace != NULL) {
            pExpIFace->ExposeCallback(fRemainingTime);
          }
        }
      } catch (...) {
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

    if (pArcDev->ContainsError(dPixelCount)) {
      pArcDev->StopExposure();
      throw std::runtime_error("Failed to read pixel count!");
    }

    ChkAbortExposure;

    if (bInReadout && pExpIFace != NULL) {
      pExpIFace->ReadCallback(dPixelCount);
    }

    ChkAbortExposure;

    // If the controller's in READOUT, then increment the timeout
    // counter. Checking for readout prevents timeouts when clearing
    // large and/or slow arrays.
    if (bInReadout && dPixelCount == dLastPixelCount) {
      dTimeoutCounter++;
    } else {
      dTimeoutCounter = 0;
    }

    ChkAbortExposure;

    if (dTimeoutCounter >= pArcDev->READ_TIMEOUT) {
      pArcDev->StopExposure();
      throw std::runtime_error("Read timeout!");
    }

    Arc_Sleep(50);
  }

  this->TotalPixelsCounted += dPixelCount;
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
                                       bool bOpenShutter) {
  int dFramesPerBuffer = 0;
  int dPCIFrameCount = 0;
  int dLastPCIFrameCount = 0;
  int dFPBCount = 0;

  /*Number of pixels to read*/
  size_t dImageSize = dRows * dCols * NDCMs * sizeof(unsigned short);
  size_t dBoundedImageSize = dImageSize;

  // Check for adequate buffer size
  if (dImageSize > pArcDev->CommonBufferSize()) {
    printf("Image dimensions [ %d x %d ] exceed buffer size: %d.\n", dCols,
           dRows, pArcDev->CommonBufferSize());
    throw std::runtime_error("Image dimensions exceed buffer size.");
  }

  // Check for valid frame count
  if (dNumOfFrames <= 0)
    throw std::runtime_error("The number of frames is less than 0.");
  if (bAbort) throw std::runtime_error("Continuous readout aborted by user.");

  // int/size_t will get promoted to size_t since its unsigned.
  dFramesPerBuffer = (int)pArcDev->CommonBufferSize() / dBoundedImageSize;

  if (bAbort) throw std::runtime_error("Continuous readout aborted by user.");

  try {
    // Skipper CCD related parameters
    if (this->CCDParams.CCDType == "SK")
      this->SetSSR();
    else
      this->CCDParams.nSkipperR = 1;


    // Set the frames-per-buffer
    int dRetVal = pArcDev->Command(TIM_ID, FPB, dFramesPerBuffer);

    if (dRetVal != DON) {
      printf("Set FramePerBuffer command failed. Reply: 0x%X\n", dRetVal);
      throw std::runtime_error("Exception thrown because FPB command failed.");
    }

    if (bAbort) throw std::runtime_error("Continuous readout aborted by user.");

    // Set the number of frames-to-take
    dRetVal = pArcDev->Command(TIM_ID, SNF, dNumOfFrames);
    if (dRetVal != DON) {
      printf("Set SetNumberOfFrames command failed. Reply: 0x%X\n", dRetVal);
      throw std::runtime_error("Exception thrown because SNF command failed.");
    }

    if (bAbort) throw std::runtime_error("Continuous readout aborted by user.");

    // Set the shutter position
    // SetOpenShutter( bOpenShutter );

    // Set the exposure time
    int dExpTime = (int)(fExpTime * 1000.0);
    dRetVal = pArcDev->Command(TIM_ID, SET, dExpTime);

    if (dRetVal != DON) {
      printf("Set exposure time failed. Reply: 0x%X\n", dRetVal);
      throw std::runtime_error("Exception thrown because SET command failed.");
    }

    //
    // Start the exposure
    //
    dRetVal = pArcDev->Command(TIM_ID, SEX);

    if (dRetVal != DON) {
      printf("Set exposure command failed. Reply: 0x%X\n", dRetVal);
      throw std::runtime_error("Exception thrown because SEX command failed.");
    }

    if (bAbort) throw std::runtime_error("Continuous readout aborted by user.");

    // Read the images
    while (dPCIFrameCount < dNumOfFrames) {
      if (bAbort)
        throw std::runtime_error("Continuous readout aborted by user.");

      dPCIFrameCount = pArcDev->GetFrameCount();

      if (bAbort)
        throw std::runtime_error("Continuous readout aborted by user.");

      if (dFPBCount >= dFramesPerBuffer) {
        dFPBCount = 0;
      }

      if (dPCIFrameCount > dLastPCIFrameCount) {
        // Call external deinterlace and fits file functions here
        if (pConIFace != NULL) {
          pConIFace->FrameCallback(dFPBCount, dPCIFrameCount, dRows, dCols,
                                   ((unsigned char*)CommonBufferVA()) +
                                       dFPBCount * dBoundedImageSize);
        }

        dLastPCIFrameCount = dPCIFrameCount;
        dFPBCount++;
      }
    }

    // Set back to single image mode
    dRetVal = pArcDev->Command(TIM_ID, SNF, 1);
    if (dRetVal != DON) {
      printf("Could not set device back to single image mode. Reply: 0x%X\n",
             dRetVal);
      throw std::runtime_error(
          "Exception thrown because SNF 1 command failed.");
    }
  } catch (std::runtime_error& e) {
    // Set back to single image mode
    pArcDev->StopContinuous();

    throw e;
  }
}
