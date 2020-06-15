#include "fitsio.h"
#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"
#include "unistd.h"

/*Function needed to convert time points to string*/
static std::string timePointAsString(const std::chrono::system_clock::time_point& tp)
{
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::string ts = std::ctime(&t);
    ts.resize(ts.size()-1);
    return ts;
}


/*Function to write an SK Merged image as a FITS file.*/
void LeachController::SaveFits(std::string outFileName, void *data_ptr, int numFrame, int totalFrames, int FPBCount)
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
    fits_write_key(fptr, TDOUBLE, "RGWIDTH", &this->CCDParams.SKRSTWidth, "Reset width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "OGWIDTH", &this->CCDParams.OGWidth, "OG Width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "SWWIDTH", &this->CCDParams.SWWidth, "SW Pulse Width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "HWIDTH", &this->CCDParams.HWidth, "H-Clock Width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "HOWIDTH", &this->CCDParams.HOverlapWidth, "H-Clock overlap Width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "VWIDTH", &this->CCDParams.VWidth, "V-Clock Width (SK only)", &status);
    fits_write_key(fptr, TDOUBLE, "VOWIDTH", &this->CCDParams.VOverlapWidth, "V-Clock overlap Width (SK only)", &status);


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
    fits_write_key(fptr, TDOUBLE, "OneTGHi", &this->ClockParams.one_tg_hi, "Transfer Gate SR1 Hi", &status);
    fits_write_key(fptr, TDOUBLE, "OneTGLo", &this->ClockParams.one_tg_lo, "Transfer Gate SR1 Lo", &status);
    fits_write_key(fptr, TDOUBLE, "TwoTGHi", &this->ClockParams.two_tg_hi, "Transfer Gate SR2 Hi", &status);
    fits_write_key(fptr, TDOUBLE, "TwoTGLo", &this->ClockParams.two_tg_lo, "Transfer Gate SR2 Lo", &status);


    fits_write_key(fptr, TDOUBLE, "HUHi", &this->ClockParams.u_hclock_hi, "U Serial Register H-Clocks Hi", &status);
    fits_write_key(fptr, TDOUBLE, "HULo", &this->ClockParams.u_hclock_lo, "U Serial Register H-Clocks Lo", &status);
    fits_write_key(fptr, TDOUBLE, "HLHi", &this->ClockParams.l_hclock_hi, "L Serial Register H-Clocks Hi", &status);
    fits_write_key(fptr, TDOUBLE, "HLLo", &this->ClockParams.l_hclock_lo, "L Serial Register H-Clocks Lo", &status);

    fits_write_key(fptr, TDOUBLE, "OneRGHi", &this->ClockParams.one_rg_hi, "Reset Gates on SR1 Hi", &status);
    fits_write_key(fptr, TDOUBLE, "OneRGLo", &this->ClockParams.one_rg_lo, "Reset Gates on SR1 Lo", &status);
    fits_write_key(fptr, TDOUBLE, "TwoRGHi", &this->ClockParams.two_rg_hi, "Reset Gates on SR2 Hi", &status);
    fits_write_key(fptr, TDOUBLE, "TwoRGLo", &this->ClockParams.two_rg_lo, "Reset Gates on SR2 Lo", &status);


    fits_write_key(fptr, TDOUBLE, "OneSWHi", &this->ClockParams.one_sw_hi, "Summing Wells on SR1 Hi", &status);
    fits_write_key(fptr, TDOUBLE, "OneSWLo", &this->ClockParams.one_sw_lo, "Summing Wells on SR1 Lo", &status);
    fits_write_key(fptr, TDOUBLE, "TwoSWHi", &this->ClockParams.two_sw_hi, "Summing Wells on SR2 Hi", &status);
    fits_write_key(fptr, TDOUBLE, "TwoSWLo", &this->ClockParams.two_sw_lo, "Summing Wells on SR2 Lo", &status);



    if(this->CCDParams.CCDType == "SK") {
        fits_write_key(fptr, TDOUBLE, "OneDGHi", &this->ClockParams.one_dg_hi, "DGs on SR1 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OneDGLo", &this->ClockParams.one_dg_lo, "DGs on SR1 Lo (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoDGHi", &this->ClockParams.two_dg_hi, "DGs on SR2 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoDGLo", &this->ClockParams.two_dg_lo, "DGs on SR2 Lo (SK only)", &status);

        fits_write_key(fptr, TDOUBLE, "OneOGHi", &this->ClockParams.one_og_hi, "OGs on SR1 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OneOGLo", &this->ClockParams.one_og_lo, "OGs on SR1 Lo (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoOGHi", &this->ClockParams.two_og_hi, "OGs on SR2 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoOGLo", &this->ClockParams.two_og_lo, "OGs on SR2 Lo (SK only)", &status);


    } else {
        double _DESPlaceHolder = -999.0;
        fits_write_key(fptr, TDOUBLE, "OneDGHi", &_DESPlaceHolder, "DGs on SR1 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OneDGLo", &_DESPlaceHolder, "DGs on SR1 Lo (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoDGHi", &_DESPlaceHolder, "DGs on SR2 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoDGLo", &_DESPlaceHolder, "DGs on SR2 Lo (SK only)", &status);

        fits_write_key(fptr, TDOUBLE, "OneOGHi", &_DESPlaceHolder, "OGs on SR1 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "OneOGLo", &_DESPlaceHolder, "OGs on SR1 Lo (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoOGHi", &_DESPlaceHolder, "OGs on SR2 Hi (SK only)", &status);
        fits_write_key(fptr, TDOUBLE, "TwoOGLo", &_DESPlaceHolder, "OGs on SR2 Lo (SK only)", &status);
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

    /*Meta keywords for continuous readout*/
    fits_write_key(fptr, TINT, "IFRAME", &numFrame, "Which frame is this in sequence (continuous exposure)", &status);
    fits_write_key(fptr, TINT, "NFRAMES", &totalFrames, "Total number of frames (continuous exposure)", &status);


    /*Write image*/
    unsigned short *pData = (unsigned short *)data_ptr;
    //void* pDataPtr = pArcDev->CommonBufferVA();
    //printf("Original ptr: %x\n", pDataPtr);
    //void* pDataPtr2 = pDataPtr+FPBCount*this->FrameMemorySize;
    //printf("Mod ptr: %x\n", pDataPtr2);
    //printf("nPixelsToWrite %d\n",nPixelsToWrite);
    //printf("Common buffer size: %d\n",pArcDev->CommonBufferSize());
    //unsigned short *pData = (unsigned short*)pArcDev->CommonBufferVA();


    if (pData==NULL)
        printf ("Why is the data a null pointer?\n");
    fits_write_img(fptr, TUSHORT, dfpixel, nPixelsToWrite, pData, &status);
    /*Done*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
}


void LeachController::AppendTarball(std::string fitsFile, std::string TarFile){

    /*Check if the Tar file exists. If not, create it*/
    bool TarballExists = ( access( TarFile.c_str(), F_OK ) != -1 );

    std::string _executeTarCommand;

    if (!TarballExists){
        _executeTarCommand = "tar --remove-files --create --file="+TarFile+" "+fitsFile;
    } else {
        _executeTarCommand = "tar --remove-files --append --file="+TarFile+" "+fitsFile;
    }

    //TODO: Sanitize inputs
    system(_executeTarCommand.c_str());

}


void LeachController::ArchiveTarball(std::string Tarball){

    /*Check if the Tar file exists. If not, create it*/
    bool TarballExists = ( access( Tarball.c_str(), F_OK ) != -1 );

    if (TarballExists){
        std::string _executeGzCommand = "gzip "+Tarball;
        //TODO: Sanitize inputs
        system(_executeGzCommand.c_str());

    } else {
        std::cout<<"Error: Could not locate tarball."<<"\n";
    }


}
