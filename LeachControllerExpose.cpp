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

    int ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows * this->CCDParams.nSkipperR * sizeof(short);

    pArcDev->MapCommonBuffer(ImageMemorySize);
    if ( pArcDev->CommonBufferSize() != BUFFER1_SIZE )
    {
        std::cerr << "Failed to map image buffer!" << std::endl;
        return 1;
    }

    //Select amplifiers and de-interlacing
    int dDeintAlg, SOS_response;
    if (this->CCDParams.AmplifierDirection == "UL") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_PARALLEL;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_LR);

    } else if (this->CCDParams.AmplifierDirection == "U") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_L);

    } else if (this->CCDParams.AmplifierDirection == "L") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_R);

    } else {
        std::cout << "The amplifier selected does not exist. Stop and verify!\n";
    }

    if (SOS_response != DON)
        std::cout << "Amplifier settings could not be applied. \n";
    else
        std::cout << "Amplifier selected. \n";

    std::cout << "Starting exposure\n";
    pArcDev->Expose(ExposeTime, this->CCDParams.dRows, this->CCDParams.dCols * this->CCDParams.nSkipperR, false,
                    &this->cExposeListener);


    if (this->CCDParams.AmplifierDirection == "LR") {
        unsigned short *pU16Buf = (unsigned short *) pArcDev->CommonBufferVA();
        std::cout << "Since amplifier selected was LR, the image will now be de-interlaced.\n";
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

