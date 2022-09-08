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

#include "../mco/mco.h"

// macros for the Atmel CANARY board

// bargraph enable with P3.2 = 0, P3.4 = 1
#define BARGRAPH_E   0xFB
// bargraph Disable with P3.2=1, P3.4=1
#define BARGRAPH_DIS 0x04
// switch enable with P3.2=1, P3.4=0
#define SWITCH_E     0xEF
// switch Disable with P3.2=1, P3.4=1
#define SWITCH_DIS   0x10

// bargraph and switch enable and disable macros
#define BARGRAPH_ENABLE	 P3 = P3 & BARGRAPH_E
#define BARGRAPH_DISABLE P3 = P3 | BARGRAPH_DIS
#define SWITCH_ENABLE	 P3 = P3 & SWITCH_E
#define SWITCH_DISABLE	 P3 = P3 | SWITCH_DIS

#define IO_WRITE_BARGRAPH  0x01
#define IO_INT1_PB         0x02
#define IO_READ_SWITCHS    0x03

/**************************************************************************
DOES:    Read the settings of the DIP switches              
RETURNS: Current value of the DIP switches                    
**************************************************************************/
BYTE read_dip_switches
  (
  void
  );

/**************************************************************************
DOES:    Outputs an 8-bit pattern on the LEDs                  
RETURNS: nothing                                                     
**************************************************************************/
void switch_leds
  (
  BYTE on_off  // set bits to turn the corresponding LEDs on
  );

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
  );

