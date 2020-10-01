#include "rpc/client.h"
#include <iostream>

int main() {
    rpc::client SRS("localhost", 20555);
    float CurrentMemVoltage = SRS.call("ReadMemVoltage").as<float>();
    //std::cout<<"Voltage: "<<a<<"\n";
    double rampTime = 5;
	printf("Ramp down to zero volts over %0.1f s....\n", rampTime);
    SRS.call("VoltageRamp", CurrentMemVoltage, 0, rampTime, true);
    return 0;
}