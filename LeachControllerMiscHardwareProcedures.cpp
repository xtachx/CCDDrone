//
// Created by Pitam Mitra on 2019-02-13.
//

#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CExpIFace.h"
#include "ArcDefs.h"


#include "LeachController.hpp"
#include "CCDControlDataTypes.hpp"


/*Click driver jumper setting*/
#define CLOCK_JUMPER 2

/* Clock driver max volts +10 and min volts -10*/
#define MAX_CLOCK 13.0
#define MIN_CLOCK -13.0


void LeachController::ApplyAllPositiveVPixelArray()
{

    this->SetDACValueClock(0, 9.0, 9.0); //Channel 0: Min 9V Max 9V
    this->SetDACValueClock(1, 9.0, 9.0); //Channel 1: Min 9V Max 9V
    this->SetDACValueClock(2, 9.0, 9.0); //Channel 2: Min 9V Max 9V

}

void LeachController::RestoreVClockVoltages(void )
{

    this->SetDACValueClock(0, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 0: V1
    this->SetDACValueClock(1, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 1: V2
    this->SetDACValueClock(2, this->ClockParams.vclock_lo, this->ClockParams.vclock_hi); //Channel 2: V3

}


void LeachController::StartupController(void )
{

    //RESET
    pArcDev->ResetController();
    //Test Data Link
    for (int i=0; i<123; i++)
    {
        if ( pArcDev->Command( TIM_ID, TDL, 0x123456 ) != 0x123456 )
        {
            std::cout<<"TIM TDL failed.\n";
            throw 10;
        }
    }

    //Load controller file
    pArcDev->LoadControllerFile(this->CCDParams.sTimFile.c_str());
    pArcDev->Command( TIM_ID, PON ); //Power ON
    pArcDev->SetImageSize(this->CCDParams.dRows,this->CCDParams.dCols); //Set image size for idle

}

void LeachController::PerformEraseProcedure(void){

    std::cout<<"Setting pixel array to (9V,9V)\n";
    this->ApplyAllPositiveVPixelArray();

    std::cout<<"Switching Vsub / relay OFF (pin 11) and wait 5 seconds.\n";
    this->CCDBiasToggle(0);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout<<"Switch Vsub / relay ON (pin 11). After 5 seconds, the clock voltages will be restored.\n";
    this->CCDBiasToggle(1);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    this->RestoreVClockVoltages();
    std::cout<<"Clock voltages restored. Erase procedure is now complete.\n";

}


int LeachController::SetSSR(void ){

        int dReply = 0;
        dReply = pArcDev->Command( TIM_ID, SSR, this->CCDParams.nSkipperR);
        if ( dReply == 0x00444F4E ) {
            return 0;
        } else {
            printf("Error setting skipper sequences: %X\n", dReply);
            return -1;
        }


}


void LeachController::ApplyNewSequencer(std::string seqFile) {
    pArcDev->LoadDeviceFile(seqFile.c_str());
}