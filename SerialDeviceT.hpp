/* **************************************************************************************
 * Code containing the CCD class that will have the entire definitions we will need
 * such as the erase procedure, the set biases and the CCD tuning etc
 *
 * by Pitam Mitra 2018 for DAMIC-M
 * **************************************************************************************/


#ifndef Serial_HPP_INCLUDED
#define Serial_HPP_INCLUDED


/*Includes*/
#include <iostream>
#include <termios.h>

class SerialDevice
{
public:
    SerialDevice();
    SerialDevice(std::string );
    ~SerialDevice();
    void WriteString(std::string s);
    void WriteChar(const char *c, const int n);
    std::string ReadLine();
    std::string ReadLineThrowR();

    /*More robust routines for stubborn processes like the cryocooler*/
    std::string RReadLine(int &, bool &);


    int USB;
    struct termios tty;

};

#endif
