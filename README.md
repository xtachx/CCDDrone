You will have to install cFITSIO. If you are using the Leach system to drive a DES or a Skipper CCD, this should work for you. For a skipper CCD, use the UW firmware. The ASM code for that is on gitlab and is not made public. Please check the pin numbering before you run the code. 

Note: You will need to re-compile the Leach kernel module with extra memory if you want to run larger CCDs or skipper CCDs with a lot of charge measurements.
