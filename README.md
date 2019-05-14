Introduction and about:
-----------------------------------------------------------

CCDDrone is a replacement for Owl in the operation of CCDs with a Leach system. This is designed to mostly work in conjunction with the super-sequencer, but you can use it with any sequencer of your choosing.

There are two parts to CCDDrone: The CMake file will first compile Leachcontroller library. You can use this with the LeachController.hpp header file to write your own code to control the Leach system. This is specially helpful if you want to run automated tests or analysis with certain CCDs. 

The descriptions of the other targets are as follows:

1. CCDDStartupAndErase: This is the first program that you will run after you turn on the Leach system. It will start up the electronics, upload the firmware and perform an erase procedure. It will also set the idle clocks. New: You can now specify the config file you want to use with ./CCDDApplyNewSettings <config file> . The default is config/Config.ini

2. CCDDPerformEraseProcedure: This will perform an erase procedure without a reset.

3. CCDDApplyNewSettings: This will apply new settings as defined in the Config.ini file (described below) and/or upload a new sequencer if you have changed the sequencer file. 

4. CCDDExpose: This performs and exposure. It will allocate memory, get the data and store it in the output file name supplied. The format to run this program is CCDDExpose <exp> <output> where <exp> is the exposure time and <output> is the output file name with the full path.


The Config.ini file:
-----------------------------------------------------------

The configuration for the acquisition is stored in the config/Config.ini file. It is divided into 3 sections. The first one is [ccd] for basic parameters, the next one is [clocks] for the clock voltages and the third one is [bias] for bias voltages. The entries in [clocks] and [bias] are self explanatory. A short description of each entry in the [ccd] section is as follows:

sequencer_loc: The location of the sequencer/firmware (.lod) file with the full path.

super_sequencer: Set this to true if you are using the SuperSequencer. Otherwise, set this to false. 

type: CCD Type being used. Possible types: DES (for DESI CCDs) and SK for Skipper CCDs.

columns: This is self explanatory. This comes from the CCD geometry. If you require an overscan, that gets added here.

rows: This is self explanatory as well. This comes from the CCD geometry. If you require an overscan, that gets added here.

NDCM: Non Destructive Charge Measurements. This is a Skipper CCD only operation and if the type=DES then this is ignored.

RG_inv: This is a relic from the past. In the old sequencers, the RG was inverted. The newer sequencers have this fixed, but should you use an older sequencer, this needs to be set to true.

AmplifierDirection: The side of the serial register where amplifier will be read. Possible values U, L, UL. 

HClkDirection: Which direction the charges should move in with respect to the H-clocks (serial register). Possible values U, L, UL. Super-sequencer only. Normally, if you have done the wiring right, this will be the same as AmplifierDirection. However, some places like U Chicago has their clocks backwards, so when you think you are moving the charges to U, they really move to L (!!). If you have one of these CCDs, then this will be the opposite of AmplifierDirection (and I personally believe that you should fix the wiring).

VClkDirection: Which direction the charges should move in with respect to the V-clocks / Pixel array. Possible values 1,2,(12). Super-sequencer only.

IntegralTime: Charge integration time in micro-seconds. Super-sequencer only.

PedestalIntgWait: Wait time (in us) before pedestal integration begins. Includes ADC refresh. Super-sequencer only.

SignalIntgWait: Wait time (in us) before signal integration begins. Super-sequencer only.

ParallelBin: Binning of the parallel clocks in the V-direction. Super-sequencer only.

SerialBin: Binning of the serial clocks in the H-directions. Super-sequencer only.




Installing:
-----------------------------------------------------------

You will have to install cFITSIO first. The way to compile this code is to run cmake and then make:

1. cmake . 
2. make

(pay attention to the dot after the cmake command). It will produce all 5 targets.

Note: You will need to re-compile the Leach kernel module with extra memory if you want to run larger CCDs or skipper CCDs with a lot of charge measurements.



Need help? 
-----------------------------------------------------------

If you need any assistance, please let me know at pitamm@uw.edu .
