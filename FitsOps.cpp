#ifndef FITSOPS_HPP
#define FITSOPS_HPP

#include "fitsio.h"
#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"


/*Function to write an SK Merged image as a FITS file.*/
void LeachController::SaveFits(std::string outFileName){

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
    fits_write_key(fptr, TBYTE, "InvRG", &this->CCDParams.InvRG, "Is RG inverted", &status);
    fits_write_key(fptr, TSTRING, "HCKDirn", (char*) this->CCDParams.HClkDirection.c_str(), "Serial register h-clock direction", &status);
    fits_write_key(fptr, TSTRING, "VCKDirn", (char*) this->CCDParams.VClkDirection.c_str(), "Vertical clock direction", &status);

    /*Write the Meta keywords - Clocks*/
    fits_write_key(fptr, TDOUBLE, "VCKHi", &this->ClockParams.vclock_hi, "V clock Hi", &status);
    fits_write_key(fptr, TDOUBLE, "VCKLO", &this->ClockParams.vclock_lo, "V clock Lo", &status);
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

    if(this->CCDParams.CCDType == "SK"){
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
    fits_write_key(fptr, TDOUBLE, "VDD", &this->BiasParams.vdd, "Vdd", &status);
    fits_write_key(fptr, TSHORT, "VidOffS", &this->BiasParams.video_offsets, "Video Offset", &status);

    if(this->CCDParams.CCDType == "SK"){
        double _SKPlaceHolder = -998.0;
        fits_write_key(fptr, TDOUBLE, "Drain", &this->BiasParams.drain, "Drain (SK Only)", &status);
        fits_write_key(fptr, TDOUBLE, "VRef", &this->BiasParams.vrefsk, "VRef", &status);
        fits_write_key(fptr, TDOUBLE, "OpG", &_SKPlaceHolder, "OpG (DES Only)", &status);
    } else {
        double _DPlaceHolder = -997.0;
        fits_write_key(fptr, TDOUBLE, "Drain", &_DPlaceHolder, "Drain (SK Only)", &status);
        fits_write_key(fptr, TDOUBLE, "VRef", &this->BiasParams.vref, "VRef", &status);
        fits_write_key(fptr, TDOUBLE, "OpG", &this->BiasParams.opg, "OpG (DES only)", &status);
    }

    /*Write image*/
    unsigned short *pData = (unsigned short *)pArcDev->CommonBufferVA();
    if (pData==NULL) printf ("Why is the data a null pointer?\n");
    fits_write_img(fptr, TUSHORT, dfpixel, nPixelsToWrite, pData, &status);
    /*Done*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
}

#endif
