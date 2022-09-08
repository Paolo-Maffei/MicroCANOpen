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
MODULE:    MCOHW591
CONTAINS:  Preliminary, limited hardware driver implementation for
           Philips 8xC591 - tested using the Phytec Rapid Development Kit
           This version was tested with the Raisonance compiler system.
           www.raisonance.com
           This version re-uses functions provided by
           www.esacademy.com/faq/progs
           If you need a full-featured 8XC591 MicroCANopen driver, contact
           support@esacademy.com
COPYRIGHT: Embedded Systems Academy, Inc. 2002-2003.
           All rights reserved. www.microcanopen.com
           This software was written in accordance to the guidelines at
	   www.esacademy.com/software/softwarestyleguide.pdf
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
LICENSE:   Users that have purchased a license for PCANopenMagic
           (www.esacademy.com/software/pcanopenmagic)
           may use this code in commercial projects.
           Otherwise only educational use is acceptable.
VERSION:   1.20, Pf/Aa/Ck 17-OCT-03
---------------------------------------------------------------------------
HISTORY:   1.20, AA 17-OCT-03, Clean up
           1.10, Pf 27-MAY-03, Cleaned file to eliminate compiler warning
                 Support MEMORY types
           1.00, Pf 07-OCT-02, First Published Version
---------------------------------------------------------------------------
PRELIMINARY VERSION

Shortcomings:
Only supports up to 4 (out of 8 possible) receive filter
  => This version can only be used with a maximum of 2 RPDOs

Only supports a transmit queue of length "1"
If queue occupied, waits until it is clear
**************************************************************************/

#include <Reg591.h>
#include "mcohw.h"

// Extended register definitions
#define BTR0                     0x06
#define BTR1                     0x07
#define ACFMODE                  0x1D
#define ACFENA                   0x1E
#define ACR0                     0x20
#define AMR0                     0x24
#define RBF                      0x60
#define TBF                      0x70

// Global timer/conter variable, incremented every millisecond
WORD MEM_NEAR gTimCnt = 0;

// global can filter count
BYTE MEM_FAR gCANFilter = 0;

/**************************************************************************
DOES:    Sets one of the four screeners (acceptance filters) of
         the CAN controller.
CAUTION: Does assume the screeners are set-up to be used as "one
         long filter for standard messages"
RETURNS: nothing
**************************************************************************/
void set_screener_std
  (
  BYTE Screener,  // 1 - 4, one of the four screeners
  WORD ID_Mask,   // Bit set: corresponding bit in ID is don't care
                  // Bit clear: corresponding bit in ID must match ID_Match
  WORD ID_Match,  // ID_Match - Match/Code value for ID 
  BYTE B1_Mask,   // Bit set: cor. bit in data byte 1 is don't care
                  // Bit clear: cor. bit in data byte 1 must match B1_Match
  BYTE B1_Match,  // Match/Code value for data byte 1
  BYTE B2_Mask,   // Bit set: cor. bit in data byte 2 is don't care
                  // Bit clear: cor. bit in data byte 2 must match B2_Match
  BYTE B2_Match   // Match/Code value for data byte 2
  )
{
  // ensure 0 <= screener <= 3
  Screener -= 1;
  Screener &= 0x3;

  // acceptance filter enable register
  CANADR = ACFENA;

  // disable all four banks
  switch (Screener)
  {
    case 0: CANDAT &= 0xFE; break;
    case 1: CANDAT &= 0xFB; break;
    case 2: CANDAT &= 0xEF; break;
    case 3: CANDAT &= 0xBF; break;
  }

  // Initialize acceptance filter - Mask
  // acceptance mask register bank "Screener"
  CANADR = AMR0 + (Screener * 8);
  CANDAT = (BYTE) (ID_Mask >> 3);
  CANDAT = (BYTE) (((ID_Mask & 0x07) << 5) | 0x1F);
  CANDAT = B1_Mask;
  CANDAT = B2_Mask;

  // Initialize acceptance filter - Code / Match
  // acceptance code register bank "Screener"
  CANADR = ACR0 + (Screener * 8);
  CANDAT = (BYTE) (ID_Match >> 3);
  CANDAT = (BYTE) (ID_Match & 0x07) << 5;
  CANDAT = B1_Match;
  CANDAT = B2_Match;

  // acceptance filter enable register
  CANADR = ACFENA;
  // enable all four banks
  switch (Screener)
  {
    case 0: CANDAT |= 0x01; break;
    case 1: CANDAT |= 0x04; break;
    case 2: CANDAT |= 0x10; break;
    case 3: CANDAT |= 0x40; break;
  }
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
  BYTE  i;

  // Check the CAN status register for received message
  if ( CANSTA & 0x01)
  {
    // Message received!

    // set address to receive buffer
    CANADR       = RBF;
    Length       = CANDAT & 0x0F;
	// read identifier
    Identifier   = (DWORD)CANDAT;
    Identifier <<= 8;
    Identifier  |= (DWORD) CANDAT;
    Identifier >>= 5;
	// set address to data bytes
    CANADR       = RBF + 3;

    // Copy message to application message buffer.
    pReceiveBuf->ID = Identifier;
    pReceiveBuf->LEN = Length;

    // Read data bytes and write to buffer
    for (i=0; i < Length; i++)
	{
	  // copy bytes
      *(BYTE *)(pReceiveBuf->BUF+i) = CANDAT;
	}

    // release receive buffer
    CANCON = 0x0C;

	// return 1, message received
    return (1);
  }
  else 
  {
    // return 0, no message received
    return (0);
  }
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

  // Prepare length code and identifier.
  Length     = pTransmitBuf->LEN;
  Identifier = (pTransmitBuf->ID << 5);

  // Wait until write access to CAN controller buffer is allowed 
  while (!(CANSTA & 0x04));
  if (!(CANSTA & 0x04))
  {
    return 0;
  }

  // Write message to transmit buffer
  // address to transmit buffer
  CANADR = TBF;
  // write length code
  CANDAT = Length;
  CANDAT = (BYTE)(Identifier >> 8);
  // write identifier
  CANDAT = (BYTE)Identifier;
  // write data bytes
  for (i=0; i < Length; i++)
  {
    CANDAT = *(BYTE *)(pTransmitBuf->BUF+i);
  }

  // Init a transmission request.
  CANCON  = 0x01;
   
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

  // make copy of current timer tick
  tmp = gTimCnt;

  // enable interrupts
  EA = 1;

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
  // get current time
  time_now = gTimCnt;
  // enable interrupts
  EA = 1;
  // ensure minimum runtime
  timestamp++;
  if (time_now > timestamp)
  {
    if ((time_now - timestamp) < 0x8000)
      return 1;
    else
      return 0;
  }
  else
  {
    if ((timestamp - time_now) > 0x8000)
      return 1;
    else
      return 0;
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
  // set timer reload value, 1ms at 12MHz, 6 clocks/cycle
  TH0 = 0xF8;
  TL0 = 0x40;
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

  // reset mode for initialization
  CANMOD = 0x01;

  // This version only supports 125kbit at 12MHz
  if (BaudRate == 125)
  {
    // BTR0 and BTR1 determine the baudrate and sample point position
    // set address to BTR0 register
    CANADR = BTR0;
    // write data to BTR0 (auto inc CANADR)
    CANDAT = 0x85;
    // set address to BTR1 register
    CANADR = BTR1;
    // write data to BTR1
    CANDAT = 0x2B;

    // the baudrate is supported
    baudrateok = 1;
  }

  // no filters configured
  gCANFilter = 0;

  // Clear all acceptance filters and masks, receive nothing
  CANADR = ACR0;
  for(i=1;i<=32;i++) CANDAT = 0;

  // Set acceptance filter mode to accept standard frames with single
  // acceptance filter.                                                   
  // acceptance filter mode register
  CANADR = ACFMODE;
  // all banks -> one long filter
  CANDAT = 0x55;
  // acceptance filter enable register
  CANADR = ACFENA;
  // enable filter for all banks
  CANDAT = 0x55;

  // release CAN controller
  CANMOD = 0x00;

  // wait until controller is active
  while (CANSTA & 0x80);
   
  // Initialize Timer interrupt here. 
  // MCOHW_TimerISR must be executed once every millisecond.
  // timer 0 stop
  TR0     =  0;
  // mode 1
  TMOD    |= 1;
  // priority
  IP0     |= 2;
  TH0     =  0xFF;
  TL0     =  0xFF;
  // timer 0 start
  TR0     =  1;
  // enable timer 0 interrupt
  ET0     =  1;

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
  if (gCANFilter > 4)
  {
    return 0;
  }
  // filter available
  else
  {
    // configure the filter
    set_screener_std(gCANFilter,0x0000,CANID,0xFF,0xFF,0xFF,0xFF); 
    return 1;
  }
}

