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
MODULE:    MCOHWCC01
CONTAINS:  Preliminary, limited hardware driver implementation for
           Atmel 89C51CC01 - tested using the Atmel demo board
	   and CANopen demo board.
           This version was tested with the Keil compiler system.
           www.keil.com
           This version re-uses functions provided by
           www.esacademy.com/faq/progs
COPYRIGHT: Embedded Systems Academy, Inc. 2002-2003
           All rights reserved. www.microcanopen.com
           This software was written in accordance to the guidelines at
	   www.esacademy.com/software/softwarestyleguide.pdf
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
LICENSE:   Users that have purchased a license for PCANopenMagic
           (www.esacademy.com/software/pcanopenmagic)
           may use this code in commercial projects.
           Otherwise only educational use is acceptable.
VERSION:   1.11, Pf/Aa/Ck 13-OCT-03
---------------------------------------------------------------------------
HISTORY:   1.11, AA 13-OCT-03, Added support for CANopen board, clean up
           1.10, Pf 27-MAY-03, Cleaned file to eliminate compiler warnings
                 Support of MEMORY types
           1.00, Ck 07-OCT-02, First Published Version
---------------------------------------------------------------------------
Known shortcoming:
Only supports a transmit queue of length "1"
If queue occupied, waits until it is clear
***************************************************************************/ 

#ifdef __RC51__
#include "c51cc01.h"   // For Raisonance
#else
#include <Reg51CC01.h> // For Keil
#endif
#include "../mco/mcohw.h"

// Atmel development boards supported by this driver
#define CANARY  0   // Atmel CANARY demo board
#define CANOPEN 1   // Atmel CANopen demo board

// specify which board type to use
// change this to match the board you are using
#define BOARDTYPE CANOPEN

// Global timer/conter variable, incremented every millisecond
WORD MEM_NEAR gTimCnt = 0;

// Counts number of filters (CAN message objects) used
BYTE MEM_FAR gCANFilter = 0;

/**************************************************************************
DOES:    Sets one of the four screeners (acceptance filters) of
         the CAN controller.
CAUTION: For the AT89C51CC01 from Atmel the screeners translate
         to individual message buffers 1-14. The parameters
         x_Mask and Bx_Match are ignored.
RETURNS: nothing
**************************************************************************/
void set_screener_std
  (
  BYTE Screener,  // 1 - 4, one of the four screeners
  WORD ID_Match   // match/code value for id
  )
{
   // select message object
   CANPAGE  = Screener << 4;
   // message id bits 3 - 10
   CANIDT1  = (ID_Match & 0x07F8) >> 3;
   // message id bits 0 - 2
   CANIDT2  = (ID_Match & 0x0007) << 5;
   // no remote request
   CANIDT4  = 0x00;
   // mask bits 3 - 10: must match
   CANIDM1  = 0xFF;
   // mask bits 0 - 2: must match
   CANIDM2  = 0xE0;
   // only accept that message id
   CANIDM4  = 0x05;
   // clear receive ok (and all other) flags
   CANSTCH  = 0x00;
   // enable msg object - message object is enabled for receive, expecting 8 bytes
   CANCONCH = 0x88;
}

/**************************************************************************
DOES:    Gets the next received CAN message and places it in
         a receive buffer                                         
RETURNS: 0 if no message received, 1 if message received and      
         copied to the buffer                                     
**************************************************************************/
BYTE MCOHW_PullMessage
  (
  CAN_MSG MEM_FAR *pReceiveBuf  // pointer to a single message sized buffer
                                // to hold the received message
  )
{
  // variable declarations
  DWORD Identifier;
  BYTE  Length;
  BYTE  i,j;

  // loop through all the receive message objects
  for (j=1; j<=gCANFilter; j++)
  {
    // select message object
    CANPAGE = j << 4;

    // check the CAN status register for received message
	// has a message been received?
    if (CANSTCH & 0x20)
    {
      // get message identifer and length
      Identifier = (unsigned int)(CANIDT1 << 3) | (CANIDT2 >> 5);
      Length     = CANCONCH & 0x0F;

 	  // copy message identifer and length to application message buffer
      pReceiveBuf->ID  = Identifier;
      pReceiveBuf->LEN = Length;
      // copy message data to application message buffer     
      for (i=0; i < Length; i++)
	  {
	    *(BYTE *)(pReceiveBuf->BUF+i) = CANMSG;
      }

      // clear receive ok flag so we can receive another message
      CANSTCH &= 0xDF;
      // re-enable message object - message object receives and is enabled
	  // 8 bytes expected
      CANCONCH = 0x88;

	  // return 1 - message received
      return (1);
    }
  }

  // return 0 - no message received
  return (0);
}

/**************************************************************************
DOES:    Transmits a CAN message                                  
RETURNS: 0 if the message could not be transmitted, 1 if the      
         message was transmitted                                  
**************************************************************************/
BYTE MCOHW_PushMessage
  (
  CAN_MSG MEM_FAR *pTransmitBuf  // pointer to buffer containing CAN
                                 // message to transmit
  )
{
  // CAN message identifier
  DWORD Identifier;
  // length of data frame
  BYTE  Length;
  // local loop counter
  BYTE  i;

  // Prepare length code and identifier
  Length     = pTransmitBuf->LEN;
  Identifier = pTransmitBuf->ID;

  // select message object 0
  CANPAGE  = 0 << 4;

  // Check if write access to CAN controller buffer is allowed
  while (!(CANSTCH & 0x40));
  // write access not allowed right now so give up
  if (!(CANSTCH & 0x40))
  {
    return 0;
  }

  // disable message object
  CANCONCH &=  0x3F;
  // clear TXOK bit
  CANSTCH  &= ~0x40;

  // store message identifier bits 0 - 10
  CANIDT1  = (Identifier & 0x07F8) >> 3;
  CANIDT2  = (Identifier & 0x0007) << 5;
  // no remote request
  CANIDT4  = 0x00;

  // write data to transmit buffer
  for (i=0; i < Length; i++)
  {
    CANMSG = pTransmitBuf->BUF[i];
  }

  // set length and enable msg object => causes transmission
  CANCONCH = 0x40 | (Length & 0x0F);

  // message sent
  return 1;
}

/**************************************************************************
DOES:    Gets the value of the current 1 millisecond system timer 
RETURNS: The current timer tick                                   
**************************************************************************/
WORD MCOHW_GetTime
  (
  void
  )
{
  WORD tmp;

  // disable interrupts
  EA = 0;

  // get copy of timer tick
  tmp = gTimCnt;

  // enable interrupts
  EA = 1;

  // return timer tick copy
  return tmp;
}

/**************************************************************************
DOES:    Checks if a moment in time has passed (a timestamp has expired)
RETURNS: 0 if timestamp has not yet expired, 1 if the             
         timestamp has expired                                    
**************************************************************************/
BYTE MCOHW_IsTimeExpired
  (
  WORD timestamp  // timestamp to check for expiration
  )
{
  WORD time_now;

  // disable interrupts
  EA = 0;
  // get a copy of the current time
  time_now = gTimCnt;
  // enable interrupts
  EA = 1;
  // to ensure the minimum runtime
  timestamp++;
  // if timestamp is less than the current time...
  if (time_now > timestamp)
  {
    // check if the timestamp has expired
    if ((time_now - timestamp) < 0x8000)
    {
      return 1;
    }
	// timestamp has not expired
	else
	{
      return 0;
    }
  }
  // if timestamp is greater than the current time...
  else
  {
    // check if the timestamp has expired
    if ((timestamp - time_now) > 0x8000)
    {
      return 1;
    }
	// timestamp has not expired
    else
    {
      return 0;
    }
  }
}

/**************************************************************************
DOES:    Timer interrupt service routine                          
         Increments the global millisecond counter tick           
         This function needs to be called once every millisecond  
RETURNS: nothing                                                     
**************************************************************************/
void MCOHW_TimerISR
  (
  void
  ) interrupt 1
{
  // stop timer 0
  TR0 = 0;
#if (BOARDTYPE == CANARY)
  // reload for 1ms at 12MHz, X2 mode
  TH0 = 0xF8;
  TL0 = 0x30;
#elif (BOARDTYPE == CANOPEN)
  // reload for 1ms at 20MHz, X2 mode
  TH0 = 0xF2;
  TL0 = 0xFB;
#endif
  // start timer 0
  TR0 = 1;

  // increment global counter
  gTimCnt++;
}

/**************************************************************************
DOES:    Initializes the CAN interface.                           
CAUTION: Does not initialize filters - nothing will be received   
         unless screeners are set using set_screener_std          
RETURNS: 0 for failed init, 1 for successful init                 
**************************************************************************/
BYTE MCOHW_Init
  (
  WORD BaudRate  // desired baudrate in kbps
  )
{
  BYTE i;
  BYTE baudrateok = 0;

  // enable X2 mode - 6 clocks/cycle
  CKCON = 0x01;

  // this version only supports 125kbps
  if (BaudRate == 125)
  {
#if (BOARDTYPE == CANARY)
    // timing for X2 mode, 12MHz, 125kbps
    CANBT1 = 0x0A;
    CANBT2 = 0x0A;
    CANBT3 = 0x1C;
#elif (BOARDTYPE == CANOPEN)
    // timing for X2 mode, 20MHz, 125kbps
    CANBT1 = 0x12;
    CANBT2 = 0x2E;
    CANBT3 = 0x18;
#endif

    // the baudrate is supported
    baudrateok = 1;
  }

  // no filters configured
  gCANFilter = 0;

  // clear all acceptance filters and masks, receive nothing
  for (i=0; i<15; i++)
  {
    // select message object
    CANPAGE  = i << 4;
    // message id bits 0 - 10
    CANIDT1  = 0xFF;
    CANIDT2  = 0xE0;
    // no remote request
    CANIDT4  = 0x00;
    // mask bits 0 - 10
    CANIDM1  = 0xFF;
    CANIDM2  = 0xE0;
    // only accept that message id
    CANIDM4  = 0x05;
    // is this a receive message object?
    if (i!=0)
    {
	  // clear receive ok and all other flags
      CANSTCH = 0x00;
    }
    // transmit message object?
    else
    {
      // set tx for first transmission
      CANSTCH = 0x40;
    }
    // disable msg object
    CANCONCH = 0x00;
  }
  
  // enable can controller
  CANGCON = 0x02;
  // wait for can controller to be enabled
  while (!(CANGSTA & 0x04));

  // initialize timer interrupt here. 
  // MCOHW_TimerISR must be executed once every millisecond.
  // stop timer 0
  TR0     =  0;
  // mode 1
  TMOD    |= 1;
#if (BOARDTYPE == CANARY)
  // reload for 1ms at 12MHz, X2 mode
  TH0 = 0xF8;
  TL0 = 0x30;
#elif (BOARDTYPE == CANOPEN)
  // reload for 1ms at 20MHz, X2 mode
  TH0 = 0xF2;
  TL0 = 0xFB;
#endif
  // start timer 0
  TR0     =  1;
  // enable timer 0
  ET0     =  1;

  // return result of initialization
  return baudrateok;
}

/**************************************************************************
DOES:    Initializes the next available filter                    
RETURNS: 0 for failed init, 1 for successful init                 
**************************************************************************/
BYTE MCOHW_SetCANFilter
  (
  WORD CANID  // identifier to receive
  )
{
  // get the number of the next available filter
  gCANFilter++;
  // if all filters used then fail
  if (gCANFilter > 14)
  {
    return 0;
  }
  // filter available
  else
  {
    // configure the filter
    set_screener_std(gCANFilter, CANID); 
    return 1;
  }
}

