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
NAME:    r591io.c                                                    
INFO:    In/Out module for the PHYTEC training board equipped with   
         PHYTEC phyCORE 591 and Philips P8x591 Microcontroller       
RIGHTS:  Embedded Systems Academy   www.esacademy.com                
---------------------------------------------------------------------------
HISTORY: V1.02    AA    17-OCT-2003 Clean up
         V1.01    Pf    07-OCT-2002 For MCO, changed include file    
         V1.00    Pf    21-FEB-2000                                  
**************************************************************************/

#include "mco.h"

#define ADDR_DIP_INPUT_PORT 0xFF80
#define ADDR_INPUT_PORT		 0xFFA0
#define ADDR_OUTPUT_PORT	 0xFFA0

/**************************************************************************
DOES:    Reading the status of push button S1 or S2               
RETURNS: 0, if button SX is not pushed down                       
         0xFF, if button is pushed down                           
**************************************************************************/
BYTE check_button
  (
  BYTE Sx  // 1 or 2 for reading button S1 or S2
  );

/**************************************************************************
DOES:    Read the settings of the 8 DIP switches S5               
RETURNS: Current value of the DIP switches S5                     
**************************************************************************/
BYTE read_dip_switches
  (
  void
  );

/**************************************************************************
DOES:    Read an analog input value at P1.2 - Potentiometer       
CAUTION: Starts a A/D conversion and WAITS until it is completed  
RETURNS: Current value of the DIP switches S5                     
**************************************************************************/
BYTE read_poti
  (
  void
  );

/**************************************************************************
DOES:    Switches LED D1 and D2 on or off                         
         Will not use LED D1 if init_timer0_dim_d1 was called     
RETURNS: none                                                     
**************************************************************************/
void switch_leds
  (
  BYTE on_off  // 0 switches both LEDs off,
               // 1 switches LED D1 on, D2 off
               // 2 switches LED D1 off, D2 on
               // 3 switches both LEDs on 
  );

/**************************************************************************
DOES:    If a program discovers a fatal error, this routine can   
         be used to display an error code to the user.            
         THIS FUNCTION NEVER RETURNS! ONLY A RESET WILL RECOVER   
         THE BOARD!
RETURNS: none                                                     
**************************************************************************/
void error_state
  (
  BYTE error  // Error value to be displayed on both LEDs as a blinking
              // pattern. Only values from 1-12 can be "reasonably"
              // recognized by counting the "blinks"
  );

