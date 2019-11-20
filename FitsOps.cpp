#ifndef FITSOPS_HPP
#define FITSOPS_HPP

#include "fitsio.h"
#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"

/*Function needed to convert time points to string*/
static std::string timePointAsString(const std::chrono::system_clock::time_point& tp)
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::string ts = std::ctime(&t);
    ts.resize(ts.size()-1);
    return ts;
}


/*Function to write an SK Merged image as a FITS file.*/
void LeachController::SaveFits(std::string outFileName)
{

    /*These are the temporary FITS variables we need to open the file
     * fptr -> fits file pointer
     * status, imgtype and nAxis are self explanatory
     * imageSizeXY stores the size of the image
     * fpixelallread is something weird about FITS that I didnt care to read too much except
     *        it needs to be {1,1} to read the entire file
     */
    fitsfile *fptr;       /* pointer to the FITS file; defined in fitsio.h */
    int status, nAxis=2;
    long fpixel[2] = {1,1};
    int dfpixel = 1;
    long nPixelsToWrite;
    long imageSizeXY[2] = { this->CCDParams.dCols*this->CCDParams.nSkipperR, this->CCDParams.dRows};
    nPixelsToWrite = imageSizeXY[0] * imageSizeXY[1];


    status = 0;         /* initialize status before calling fitsio routines */
    fits_create_file(&fptr, outFileName.c_str(), &status);
    fits_create_img(fptr, USHORT_IMG, nAxis, &imageSizeXY[0], &status);
    int nullValue=0;

    /*Write processed comment*/
    std::string sKFixedCmt = "This image was taken by a DAMIC UW CCD. "
                             "The various settings used are stored as keys in the FITS file."
                             "Processed by CCDDrone - Pitam Mitra @ UW. If you have questions, please send them to pitamm@uw.edu";
    fits_write_comment(fptr, sKFixedCmt.c_str(), &status);

    /* Write the Meta keywords - CCD*/
    std::string SequencerUsed = "ASMFILE: Sequencer file used "+this->CCDParams.sTimFile;

    fits_write_key(fptr, TSTRING, "CCDType", (char*) this->CCDParams.CCDType.c_str(), "CCD Type (DES or SK)", &status);
    fits_write_key(fptr, TFLOAT, "Exp", &this->CCDParams.fExpTime, "Exposure time (s)", &status);
    fits_write_key(fptr, TSHORT, "NDCMs", &this->CCDParams.nSkipperR, "Number of charge measurements", &status);
    fits_write_key(fptr, TSTRING, "AMPL", (char*) this->CCDParams.AmplifierDirection.c_str(), "Amplifier(s) used", &status);
    fits_write_comment(fptr, SequencerUsed.c_str(), &status);
    fits_write_key(fptr, TBYTE, "SUPERSE", &this->CCDParams.super_sequencer, "Super sequencer (SSeq) used?", &status);
    fits_write_key(fptr, TBYTE, "InvRG", &this->CCDParams.InvRG, "Is RG inverted", &status);
    fits_write_key(fptr, TSTRING, "HCKDirn", (char*) this->CCDParams.HClkDirection.c_str(), "Serial register h-clock direction (SSEq only)", &status);
    fits_write_key(fptr, TSTRING, "VCKDirn", (char*) this->CCDParams.VClkDirection.c_str(), "Vertical clock direction (SSeq only)", &status);
    fits_write_key(fptr, TDOUBLE, "ITGTIME", &this->CCDParams.IntegralTime, "Integration time (SSeq only)", &status);
    fits_write_key(fptr, TINT, "VidGain", &this->CCDParams.Gain, "Video gain", &status);
    fits_write_key(fptr, TINT, "ITGSpd", &this->CCDParams.ItgSpeed, "Integrator speed (0=slow, 1=fast)", &status);
    fits_write_key(fptr, TDOUBLE, "PRETIME", &this->CCDParams.PedestalIntgWait, "Pedestal settling + video ADC refresh time", &status);
    fits_write_key(fptr, TDOUBLE, "POSTIME", &this->CCDParams.SignalIntgWait, "Signal settling time", &status);
    fits_write_key(fptr, TDOUBLE, "DGWIDTH", &this->CCDParams.DGWidth, "DG Width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "RGWIDTH", &this->CCDParams.SKRSTWidth, "Skipping reset width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "OGWIDTH", &this->CCDParams.OGWidth, "OG Width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "SWWIDTH", &this->CCDParams.SWWidth, "SW Pulse Width (SK only)", &status);


    fits_write_key(fptr, TINT, "NPBIN", &this->CCDParams.ParallelBin, "Binning in the V-direction (parallel clocks)", &status);
    fits_write_key(fptr, TINT, "NSBIN", &this->CCDParams.SerialBin, "Binning in the H-direction (serial clocks)", &status);
    fits_write_key(fptr, TSTRING, "SecStg", (char*) this->CCDParams.SecondStageVersion.c_str(), "Second stage board revision (SSeq only)", &status);


    /*Write the Meta keywords - Clocks*/
    fits_write_key(fptr, TDOUBLE, "OneVCKHi", &this->ClockParams.one_vclock_hi, "V1 clock Hi", &status);
    fits_write_key(fptr, TDOUBLE, "OneVCKLo", &this->ClockParams.one_vclock_lo, "V1 clock Lo", &status);

    if (this->CCDParams.SecondStageVersion == "UW2") {
        fits_write_key(fptr, TDOUBLE, "TwoVCKHi", &this->ClockParams.two_vclock_hi, "V2 clock Hi", &status);
        fits_write_key(fptr, TDOUBLE, "TwoVCKLo", &this->ClockParams.two_vclock_lo, "V2 clock Lo", &status);
    } else {
        double _TwoVCKPlaceholder = -996.0;
        fits_write_key(fptr, TDOUBLE, "TwoVCKHi", &_TwoVCKPlaceholder, "V2 clock Hi", &status);
        fits_write_key(fptr, TDOUBLE, "TwoVCKLo", &_TwoVCKPlaceholder, "V2 clock Lo", &status);
    }
    fits_write_key(fptr, TDOUBLE, "TGHi", &this->ClockParams.tg_hi, "Transfer Gate Hi", &status);
    fits_write_key(fptr, TDOUBLE, "TGLo", &this->ClockParams.tg_lo, "Transfer Gate Lo", &status);

    fits_write_key(fptr, TDOUBLE, "HUHi", &this->ClockParams.u_hclock_hi, "U Serial Register H-Clocks Hi", &status);
    fits_write_key(fptr, TDOUBLE, "HULo", &this->ClockParams.u_hclock_lo, "U Serial Register H-Clocks Lo", &status);
    fits_write_key(fptr, TDOUBLE, "HLHi", &this->ClockParams.l_hclock_hi, "L Serial Register H-Clocks Hi", &status);
    fits_write_key(fptr, TDOUBLE, "HLLo", &this->ClockParams.l_hclock_lo, "L Serial Register H-Clocks Lo", &status);

    fits_write_key(fptr, TDOUBLE, "RGHi", &this->ClockParams.rg_hi, "Reset Gate Hi", &status);
    fits_write_key(fptr, TDOUBLE, "RGLo", &this->ClockParams.rg_lo, "Reset Gate Lo", &status);
    fits_write_key(fptr, TDOUBLE, "SWHi", &this->ClockParams.sw_hi, "Summing Well Hi", &status);
    fits_write_key(fptr, TDOUBLE, "SWLo", &this->ClockParams.sw_lo, "Summing Well Lo", &status);

    if(this->CCDParams.CCDType == "SK") {
        fits_write_key(fptr, TDOUBLE, "DGHi", &this->ClockParams.dg_hi, "DG Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "DGLo", &this->ClockParams.dg_lo, "DG Lo (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OGHi", &this->ClockParams.og_hi, "OG Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OGLo", &this->ClockParams.og_lo, "OG Lo (SK only)", &status);
    } else {
        double _DESPlaceHolder = -999.0;
        fits_write_key(fptr, TDOUBLE, "DGHi", &_DESPlaceHolder, "DG Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "DGLo", &_DESPlaceHolder, "DG Lo (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OGHi", &_DESPlaceHolder, "OG Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OGLo", &_DESPlaceHolder, "OG Lo (SK only)", &status);
    }

    /*Write the Meta keywords - Biases*/
    fits_write_key(fptr, TDOUBLE, "BATTR", &this->BiasParams.battrelay, "Battery box relay TTL Line", &status);
    fits_write_key(fptr, TDOUBLE, "VDD1", &this->BiasParams.vdd_1, "Vdd", &status);
    fits_write_key(fptr, TDOUBLE, "VDD2", &this->BiasParams.vdd_2, "Vdd", &status);
    fits_write_key(fptr, TSHORT, "VidOffL", &this->BiasParams.video_offsets_L, "Video pedestal offset L", &status);
    fits_write_key(fptr, TSHORT, "VidOffU", &this->BiasParams.video_offsets_U, "Video pedestal offset U", &status);



    fits_write_key(fptr, TDOUBLE, "Drain1", &this->BiasParams.drain_1, "Drain1 (Relevant to SK)", &status);
    fits_write_key(fptr, TDOUBLE, "Drain2", &this->BiasParams.drain_2, "Drain2 (Relevant to SK)", &status);
    fits_write_key(fptr, TDOUBLE, "VRef1", &this->BiasParams.vref_1, "VRef", &status);
    fits_write_key(fptr, TDOUBLE, "VRef2", &this->BiasParams.vref_2, "VRef", &status);
    fits_write_key(fptr, TDOUBLE, "OpG1", &this->BiasParams.opg_1, "OpG1 (Relevant to DES)", &status);
    fits_write_key(fptr, TDOUBLE, "OpG2", &this->BiasParams.opg_2, "OpG2 (Relevant to DES)", &status);



    /*Write the Meta keywords for time*/
    std::string ProgStart = timePointAsString(this->ClockTimers.ProgramStart);
    std::string ExpStart = timePointAsString(this->ClockTimers.ExpStart);
    std::string ReadOutStart = timePointAsString(this->ClockTimers.Readoutstart);
    std::string ReadOutEnd = timePointAsString(this->ClockTimers.ReadoutEnd);

    fits_write_key(fptr, TSTRING, "ProgStrt", (char*) ProgStart.c_str(), "Program start time", &status);
    fits_write_key(fptr, TSTRING, "ExpStart", (char*) ExpStart.c_str(), "Exposure start time", &status);
    fits_write_key(fptr, TSTRING, "RdStrt", (char*) ReadOutStart.c_str(), "Readout start time", &status);
    fits_write_key(fptr, TSTRING, "RdEnd", (char*) ReadOutEnd.c_str(), "Readout end time", &status);

    fits_write_key(fptr, TDOUBLE, "MExp", &this->ClockTimers.MeasuredExp, "Measured exposure time (ms)", &status);
    fits_write_key(fptr, TDOUBLE, "MRead", &this->ClockTimers.MeasuredReadout, "Measured readout time (ms)", &status);

    /*Write image*/
    unsigned short *pData = (unsigned short *)pArcDev->CommonBufferVA();
    if (pData==NULL)
        printf ("Why is the data a null pointer?\n");
    fits_write_img(fptr, TUSHORT, dfpixel, nPixelsToWrite, pData, &status);
    /*Done*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
}

#endif
