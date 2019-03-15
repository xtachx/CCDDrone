#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>

#include "LeachController.hpp"
#include <chrono>
#include <thread>


// ------------------------------------------------------
//  Main program
// ------------------------------------------------------
int main( int argc, char **argv )
{


    std::cout << "This code will perform an erase procedure.\n"
              << "The process starts in 10 seconds.\n";

    std::this_thread::sleep_for(std::chrono::seconds(10));


	LeachController _ThisRunControllerInstance("config/FirstTest.ini");

	/*First, check if the settings file has changed in any way*/
	int _CCDSettingsStatus = _ThisRunControllerInstance.LoadAndCheckForSettingsChange();

    if (_CCDSettingsStatus == 0){
        std::this_thread::sleep_for(std::chrono::seconds(5));
        _ThisRunControllerInstance.PerformEraseProcedure();
        _ThisRunControllerInstance.IdleClockToggle();
        std::cout<<"Leach system is now ready to take data.\n";
    } else {
        std::cout<<"Erase procedure was not performed. Please resolve the conflicts in the config section first.\n";
    }





}

