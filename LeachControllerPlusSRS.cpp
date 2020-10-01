/* *********************************************************************
 * This file contains miscellaneous procedures that are required for
 * operation of a CCD. These processes include things like the Charge Erase,
 * CCD Purge, set the clock directions and ccd startup.
 *
 * Created by Pitam Mitra on 2019-02-13.
 * *********************************************************************
 */

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

#include "rpc/client.h"



void LeachController::PerformEraseProcedureWithSRS(void)
{
    std::cout<<"Using Stanford Research Systems DC205 Power Supply for VSub\n";
    rpc::client SRSClient("localhost", 20555);

    float rmpdownS = this->BiasParams.vsub/this->BiasParams.rampdownrate;
    float rmpupS = this->BiasParams.vsub/this->BiasParams.rampuprate;
    
    int turnoffMS = (int)(1000.*(1. - this->BiasParams.turnoffvoltage/this->BiasParams.vsub)*rmpdownS);
    int remainderMS = (int)(1000.*(this->BiasParams.turnoffvoltage/this->BiasParams.vsub)*rmpdownS);

    int hldtimeMS = (int)(1000.*this->BiasParams.holdtime);

    int rsttimeMS = (int)(1000.*(this->BiasParams.restartvoltage/this->BiasParams.vsub)*rmpupS);
    int rmpupremainderMS = (int)(1000.*(1. - this->BiasParams.restartvoltage/this->BiasParams.vsub)*rmpupS);

    //std::cout << turnoffMS << "; " << remainderMS << "; " << hldtimeMS << "; " << rsttimeMS << "; " << rmpupremainderMS << "; \n";



    std::cout<<"Ramping Vsub OFF over " << rmpdownS << " seconds. Turning off clocks " << ((double)turnoffMS)/1000. << " seconds into this ramp. Holding low for " << this->BiasParams.holdtime << "seconds \n";
    //std::cout<<"Paolo: Commented some printout to avoid timing issues in the erase, wait till it says the procedure is completed  \n";
    
    // Ramp voltage down
    //float CurrentVoltage = SRSClient.call("ReadPSVoltage");
    float CurrentVoltage = SRSClient.call("ReadMemVoltage").as<float>();
    SRSClient.call("VoltageRamp",CurrentVoltage, 0, rmpdownS, true)
    std::this_thread::sleep_for(std::chrono::milliseconds(turnoffMS));
    std::cout<<"Setting pixel array to +V\n";
    this->ApplyAllPositiveVPixelArray();
    std::this_thread::sleep_for(std::chrono::milliseconds(remainderMS));
    std::this_thread::sleep_for(std::chrono::milliseconds(hldtimeMS));

    std::cout<<"Ramping Vsub ON over " << rmpupS << " seconds, " << rsttimeMS << " ms into which the clock voltages will be restored.\n";
    // Ramp voltage up
    SRSClient.call("VoltageRamp",0, this->BiasParams.vsub, rmpupS, true)
    std::this_thread::sleep_for(std::chrono::milliseconds(rsttimeMS));
    this->RestoreVClockVoltages();
    std::this_thread::sleep_for(std::chrono::milliseconds(rmpupremainderMS));
    std::cout<<"Clock voltages restored. Erase procedure is now complete.\n";
}

void LeachController::SetVSUB(float VSub)
{

    rpc::client SRSClient("localhost", 20555);
    // this->SRSSupply->WritePSOutput(0);
    float currentVoltage = SRSClient.call("ReadMemVoltage").as<float>();
    float deltaVoltage = VSub - currentVoltage;
    float rampTime = 1;
    if(deltaVoltage > 0){
        rampTime = deltaVoltage / this->BiasParams.rampuprate;
    }else{
        rampTime = -1 * deltaVoltage / this->BiasParams.rampdownrate;
    }

    SRSClient.call("VoltageRamp",currentVoltage, VSub, rampTime, true);
}
