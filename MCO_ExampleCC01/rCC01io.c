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
NAME:    rCC01io.c                                                   
INFO:    In/Out module for the Atmel CANary evaluation board and Atmel
         CANopen demo board, both equipped with a T89C51CC01
		 microcontroller.                 
RIGHTS:  Embedded Systems Academy   www.esacademy.com                
---------------------------------------------------------------------------
DETAILS:                                                             
---------------------------------------------------------------------------
HISTORY: V1.01 AA 15-OCT-2003 Added support for Atmel CANopen board
         V1.00 Kl 09-OCT-2002                                  
**************************************************************************/

#ifdef __RC51__
#include "c51cc01.h" // For Raisonance
#else
#include "Reg51CC01.h" // For Keil
#endif
#include "../mco/mco.h"
#include "rCC01io.h"

// Atmel development boards supported by this driver
#define CANARY  0   // Atmel CANARY demo board
#define CANOPEN 1   // Atmel CANopen demo board

// specify which board type to use
// change this to match the board you are using
#define BOARDTYPE CANOPEN

/**************************************************************************
DOES:    Read the settings of the DIP switches              
RETURNS: Current value of the DIP switches                    
**************************************************************************/
BYTE read_dip_switches
  (
  void
  )
{
  BYTE dip;

#if (BOARDTYPE == CANARY)
  // enable the switches
  SWITCH_ENABLE;
  // read the switches
  dip = P1 & 0x1F;
  // disable the switches
  SWITCH_DISABLE;
#elif (BOARDTYPE == CANOPEN)
  // read the four switches
  dip = P2 & 0x0F;
#endif

  // return the value read from the switches
  return (dip);
}

/**************************************************************************
DOES:    Outputs an 8-bit pattern on the LEDs                 
RETURNS: nothing                                                     
**************************************************************************/
void switch_leds
  (
  BYTE led7_0  // set bits to turn the corresponding LEDs on
  )
{
#if (BOARDTYPE == CANARY)
   // enable the bargraph LED display
   BARGRAPH_ENABLE;	 
   // output to the LED display
   P1 = led7_0;  
   // disable the bargraph LED display
   BARGRAPH_DISABLE;
#elif (BOARDTYPE == CANOPEN)
   // output to the four LEDs
   P0 = led7_0 & 0x0F;
#endif
}

/**************************************************************************
DOES:    If a program discovers a fatal error, this routine can   
         be used to display an error code to the user.            
         THIS FUNCTION NEVER RETURNS! ONLY A RESET WILL RECOVER   
         THE BOARD!                                               
RETURNS: nothing                                          
**************************************************************************/
void error_state
  (
  BYTE error  // error value to be displayed on all LEDs as a blinking
              // pattern. Only values 1 - 12 can be reasonably
              // recognized by counting the blinks
  )
{
  WORD i,j;

  // disable all interrupts
  EA = 0;

  // infinate loop
  while (1)
  {
    // loop the necessary number of times
    for (j = 1; j <= error; j++)
    {
      // all leds on
      for (i = 0; i <= 20000; i++)
      {
        switch_leds(0xFF);
      }
      // all leds off
      for (i = 0; i <= 20000; i++)
      {
        switch_leds(0x00);
      }
    }
    // delay before next loop
    // all leds off
    for (i = 0; i <= 50000; i++)
    {
      switch_leds(0x00);
    }
  }
}

