/* *********************************************************************
 * This file contains all the exposure related routines of the Leach CCD
 * control - CCDDrone.
 * *********************************************************************
 */

#include "LeachController.hpp"

#include <string>
#include <iostream>

#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CExpIFace.h"
#include "ArcDefs.h"


/* *********************************************************************
 * This is the CCD exposure routine. This is common to both Skipper
 * and DES. In case of DES, it counts on nSkipperR being = 1.
 * *********************************************************************
 */

int LeachController::ExposeCCD(int ExposeTime) {

    int ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows * this->CCDParams.nSkipperR * sizeof(unsigned short);

    //pArcDev->UnMapCommonBuffer();
    pArcDev->SetImageSize( this->CCDParams.dRows, this->CCDParams.dCols*this->CCDParams.nSkipperR );
    pArcDev->ReMapCommonBuffer(ImageMemorySize);

    printf("Rows %d, Cols %d \n",pArcDev->GetImageRows(), pArcDev->GetImageCols());

    if ( pArcDev->CommonBufferSize() < ImageMemorySize )
    {
        std::cout<<"Common buffer size: "<<pArcDev->CommonBufferSize()<<"  | Image memory requirement: "<<ImageMemorySize<<"\n";
        std::cerr << "Failed to map image buffer!" << std::endl;
        return 1;
    }

    //Select amplifiers and de-interlacing
    int dDeintAlg;

    if (this->CCDParams.AmplifierDirection == "UL") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_SERIAL;
    } else if (this->CCDParams.AmplifierDirection == "U") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
    } else if (this->CCDParams.AmplifierDirection == "L") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
    } else {
        std::cout << "The amplifier selected does not exist. Interlacing is not set. Stop and verify!\n";
    }


    std::cout << "Starting exposure\n";
    pArcDev->Expose(ExposeTime, this->CCDParams.dRows, this->CCDParams.dCols * this->CCDParams.nSkipperR, false,
                    &this->cExposeListener);
    std::cout << "\nExposure complete.\n";

    if (this->CCDParams.AmplifierDirection == "UL" || this->CCDParams.AmplifierDirection == "LU") {
        unsigned short *pU16Buf = (unsigned short *) pArcDev->CommonBufferVA();
        std::cout << "Since amplifier selected was UL / LU, the image will now be de-interlaced.\n";
        arc::deinterlace::CArcDeinterlace cDlacer;
        cDlacer.RunAlg(pU16Buf, this->CCDParams.dRows, this->CCDParams.dCols * this->CCDParams.nSkipperR, dDeintAlg);
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
void LeachController::PrepareAndExposeCCD(int ExposureTime, unsigned short *ImageBuffer) {

    try {
        if (this->CCDParams.CCDType == "SK") {
            this->SetSSR();
            this->ExposeCCD(ExposureTime);
        } else {
            this->CCDParams.nSkipperR = 1;
            this->ExposeCCD(ExposureTime);
        }

        ImageBuffer = (unsigned short *) this->pArcDev->CommonBufferVA();

    }
    catch (std::runtime_error &e) {
        std::cout << "failed!" << std::endl;
        std::cerr << std::endl << e.what() << std::endl;

        if (pArcDev->IsReadout()) {
            pArcDev->StopExposure();
        }

        //pArcDev->Close();
    }
    catch (...) {
        std::cerr << std::endl << "Error: unknown exception occurred!!!" << std::endl;

        if (pArcDev->IsReadout()) {
            pArcDev->StopExposure();
        }

        //pArcDev->Close();
    }


}

