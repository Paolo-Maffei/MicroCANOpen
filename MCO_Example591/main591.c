/**************************************************************************
ORIGINAL MICROCANOPEN FROM 2003 AS DESCRIBED IN THE BOOK
"EMBEDDED NETWORKING WITH CAN AND CANOPEN."

NOTE: CANOPEN AND MICROCANOPEN HAVE GREATLY EVOLVED OVER THE LAST
YEARS. THIS IMPLEMENTATION IS FOR REFERENCE, EDUCATIONAL AND STRICTLY
NON-COMMERCIAL PURPOSES ONLY.

SEE HTTP://WWW.CANOPENSTORE.COM/PIP/MICROCANOPEN-PLUS.HTML FOR THE
LATEST, RECOMMENDED VERSION.
***************************************************************************/ 

/**************************************************************************
MODULE:    MAIN
CONTAINS:  Example application using MicroCANopen
           Written for Phytec phyCORE 591 with development board
COPYRIGHT: Embedded Systems Academy, Inc. 2002 - 2003
           All rights reserved. www.microcanopen.com
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
LICENSE:   Users that have purchased a license for PCANopenMagic
           (www.esacademy.com/software/pcanopenmagic)
           may use this code in commercial projects.
           Otherwise only educational use is acceptable.
VERSION:   1.20, Pf/Aa/Ck/DM 13-OCT-03
---------------------------------------------------------------------------
HISTORY:   1.20, Pf 13-OCT-03, Support of a process image
           1.10, Pf 27-MAY-03, support of MEMORY types
           1.01, DM 16-DEC-02, "esa_defs.h" removed, Watchdog timer reset
   	   1.00, CK 07-OCT-02, First Published Version
***************************************************************************/ 

#include "../mco/mco.h"
#include "r591io.h"
#include <reg591.h>

// external declaration for the process image array
extern BYTE MEM_NEAR gProcImg[];

/**************************************************************************
DOES:    The main function
RETURNS: nothing
**************************************************************************/
void main
  (
  void
  )
{
  // Reset/Initialize CANopen communication
  MCOUSER_ResetCommunication();

  // end of initialization, enable all interrupts
  EA = 1;

  // foreground loop
  while(1)
  {
    // Update process data
    // First digital inputs are real I/O
    gProcImg[IN_digi_1] = read_dip_switches();
    gProcImg[IN_digi_2] = check_button(1);
    gProcImg[IN_digi_3] = check_button(2);
    
    // output first digital outputs to LEDs
    switch_leds(gProcImg[OUT_digi_1]); 

    // echo all other I/O values from input to
    // output
    // digital
    gProcImg[IN_digi_4] = gProcImg[OUT_digi_4];

    // first analog input is real I/O
    gProcImg[IN_ana_1] = 0; // lo byte
    gProcImg[IN_ana_1+1] = read_poti(); // hi byte

    // echo all other I/O values from input to output
    // analog
    gProcImg[IN_ana_2]   = gProcImg[OUT_ana_2];
    gProcImg[IN_ana_2+1] = gProcImg[OUT_ana_2+1];

    // Operate on CANopen protocol stack
    MCO_ProcessStack();
  } // end of while(1)
} // end of main

