#ifndef FITSOPS_HPP
#define FITSOPS_HPP

#include "fitsio.h"
#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"

class FitsOps{

private:

    /*These are the temporary FITS variables we need to open the file
     * fptr -> fits file pointer
     * status, imgtype and nAxis are self explanatory
     * imageSizeXY stores the size of the image
     * fpixelallread is something weird about FITS that I didnt care to read too much except
     *        it needs to be {1,1} to read the entire file
     */
    fitsfile *fptr;       /* pointer to the FITS file; defined in fitsio.h */
    int status, nAxis;
    long fpixel[2];
    int dfpixel;
    long nPixelsToWrite;
    long imageSizeXY[2];
    int nullValue;

public:

    FitsOps(std::string outFileName, CCDVariables &_CCDParams, BiasVariables &_BiasParams, ClockVariables &_ClockParams, TimeVariables &_ClockTimers);
    ~FitsOps();

    CCDVariables CCDParams;
    BiasVariables BiasParams;
    ClockVariables ClockParams;
    TimeVariables ClockTimers;

    int WriteHeader(void );
    int WritePostExposureInfo(void);
    int WriteData(long _StartRow, long _StartColumn, long _NumBlockRows, long _NumBlockColumns, unsigned short* ImageBufferVLoc);

};

#endif
