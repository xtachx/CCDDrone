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
 * This is the DES CCD exposure routine. This will not automatically
 * try to set an SSR / skipper repeat value.
 * Additionally, the rows and columns will be their native values
 * *********************************************************************
 */

void LeachController::ExposeCCD_DES(int ExposeTime) {

    int ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows * sizeof(short);
    pArcDev->MapCommonBuffer(ImageMemorySize);

    //Select amplifiers and de-interlacing
    int dDeintAlg, SOS_response;
    if (this->CCDParams.AmplifierDirection == "LR") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_PARALLEL;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_LR);

    } else if (this->CCDParams.AmplifierDirection == "L") {
        dDeintAlg = arc::deinterlace::CArcDeinterlace::DEINTERLACE_NONE;
        SOS_response = pArcDev->Command(TIM_ID, SOS, AMP_L);

    } else if (this->CCDParams.AmplifierDirection == "R") {
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
    pArcDev->Expose(ExposeTime, this->CCDParams.dRows, this->CCDParams.dCols, false, &this->cExposeListener);

    //Perform de-interlacing only if LR amplifiers are selected.
    if (this->CCDParams.AmplifierDirection == "LR") {
        std::cout << "Since amplifier selected was LR, the image will now be de-interlaced.\n";
        arc::deinterlace::CArcDeinterlace cDlacer;
        unsigned short *pU16Buf = (unsigned short *) pArcDev->CommonBufferVA();
        cDlacer.RunAlg(pU16Buf, this->CCDParams.dRows, this->CCDParams.dCols, dDeintAlg);
    }

}


void LeachController::ExposeCCD_SK(int ExposeTime) {

    int ImageMemorySize = this->CCDParams.dCols * this->CCDParams.dRows * this->CCDParams.nSkipperR * sizeof(short);
    //Implement a memory size check here.
    pArcDev->MapCommonBuffer(ImageMemorySize);

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


}

void LeachController::ExposeCCD(int ExposureTime, unsigned short *ImageBuffer) {

    try {
        if (this->CCDParams.CCDType == "SK") this->ExposeCCD_SK(ExposureTime);
        else this->ExposeCCD_DES(ExposureTime);

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

