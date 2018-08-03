/* *****************************************************************************
 * Bias reset and flush header
 *
 *******************************************************************************/


#ifndef CCDChargeFlushReset_HPP_INCLUDED
#define CCDChargeFlushReset_HPP_INCLUDED


/*Includes*/
#include <iostream>
#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "CArcPCI.h"
#include "CArcDeinterlace.h"
#include "CArcFitsFile.h"
#include "CExpIFace.h"

int ClockVoltToADC(double );
void CCDChargeFlushReset(arc::device::CArcDevice** );
void CCDRestoreBiasVoltages(arc::device::CArcDevice** );
void CCDRestoreClockVoltages(arc::device::CArcDevice** );

void SetDACValue(arc::device::CArcDevice** , int , int , int );
void SetDACValueBias(arc::device::CArcDevice** , int, int);


#endif
