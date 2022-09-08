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
MODULE:    MCO
CONTAINS:  MicroCANopen implementation
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
VERSION:   2.00, Pf/Aa/Ck 21-OCT-03
---------------------------------------------------------------------------
HISTORY:   2.00, Pf 21-OCT-03, Minor changes to pass conformance test
           1.20, Pf 19-AUG-03, Code changed to use process image
                 Bug fix for incorrect inhibit time handling
           1.10, Pf 27-MAY-03, Bug fixes in OD (hi byte was corrupted)
                 OD changed to indefinite length
		         Support of define controled MEMORY types
           1.01, Pf 17-DEC-02, Made Object Dictionary more readable
           1.00, Pf 07-OCT-02, First Published Version
***************************************************************************/ 

#ifndef _MCO_H
#define _MCO_H

/**************************************************************************
DEFINES: MEMORY TYPES USED
**************************************************************************/

// CONST Object Dictionary Data
#define MEM_CONST const code
// Process data and frequently used variables
#define MEM_NEAR data
// Seldomly used variables
#define MEM_FAR xdata

/**************************************************************************
DEFINES: CONST ENTRIES IN OBJECT DICTIONARY
Modify these for your application
**************************************************************************/

#define OD_DEVICE_TYPE   0x000F0191L 
#define OD_VENDOR_ID     0x00455341L
#define OD_PRODUCT_CODE  0x00010002L
#define OD_REVISION      0x00010020L

// The following are optional and can also be left "undefined"
#define OD_SERIAL        0xFFFFFFFFL

/**************************************************************************
DEFINES: Definition of the process image
Modify these for your application
**************************************************************************/

// Define the size of the process image
#define PROCIMG_SIZE 16

// Define process variables: offsets into the process image 
// Digital Input 1
#define IN_digi_1 0x00
// Digital Input 2
#define IN_digi_2 0x01
// Digital Input 3
#define IN_digi_3 0x02
// Digital Input 4
#define IN_digi_4 0x03

// Analog Input 1
#define IN_ana_1 0x04
// Analog Input 2
#define IN_ana_2 0x06

// Digital Output 1
#define OUT_digi_1 0x08
// Digital Output 2
#define OUT_digi_2 0x09
// Digital Output 3
#define OUT_digi_3 0x0A
// Digital Output 4
#define OUT_digi_4 0x0B

// Analog Output 1
#define OUT_ana_1 0x0C
// Analog Output 2
#define OUT_ana_2 0x0E

/**************************************************************************
DEFINES: ENABLING/DISABLING CODE FUNCTIONALITY
**************************************************************************/

// Maximum number of transmit PDOs (0 to 4)
#define NR_OF_TPDOS 2

// Maximum number of receive PDOs (0 to 4)
#define NR_OF_RPDOS 2

// If defined, 1 or more TPDOs use the event timer
#define USE_EVENT_TIME

// If defined, 1 or more TPDOs are change-of-state and use the inhibit timer
#define USE_INHIBIT_TIME

// If defined, the PDO parameters are added to the Object Dictionary
// Entries must be added to the SDOResponseTable in user_xxxx.c
#define PDO_IN_OD

// If defined, the Process Data is accesible via SDO requests
// Entries must be added to the ODProcTable in user_xxxx.c
#define PROCIMG_IN_OD

// If defined, OD entry [1017,00] is supported with SDO read/write accesses
// This is also an example on how to implement dynamic/variable OD entries
#define DYNAMIC_HEARTBEAT

// If defined, node starts up automatically (does not wait for NMT master)
//#define AUTOSTART

// If defined, all parameters passed to functions are checked for consistency. 
// On failures, the user function MCOUSER_FatalError is called.
#define CHECK_PARAMETERS

/**************************************************************************
END OF CUSTOMIZATION AREA - DON'T CHANGE ANYTHING BELOW
**************************************************************************/

#ifdef PROCIMG_IN_OD
// Defines for access type in OD entries
#define ODRD 0x10
#define ODWR 0x20
#endif // PROCIMG_IN_OD

#ifndef USE_EVENT_TIME
  #ifndef USE_INHIBIT_TIME
#error At least one, USE_EVENT_TIME or USE_INHIBIT_TIME must be defined!
  #endif
#endif

#if (NR_OF_RPDOS == 0)
  #if (NR_OF_TPDOS == 0)
#error At least one PDO must be defined!
  #endif
#endif

#if ((NR_OF_TPDOS > 4) || (NR_OF_RPDOS > 4))
#error This configuration was never tested by ESAcademy!
#endif

/**************************************************************************
GLOBAL TYPE DEFINITIONS
**************************************************************************/

// Standard data types
#define BYTE  unsigned char
#define WORD  unsigned int
#define DWORD unsigned long

// Boolean expressions
#define BOOLEAN unsigned char
#define TRUE 0xFF
#define FALSE 0

// Data structure for a single CAN message 
typedef struct
{
  WORD ID;                  // Message Identifier 
  BYTE LEN;                 // Data length (0-8) 
  BYTE BUF[8];              // Data buffer 
} CAN_MSG;

// This structure holds all node specific configuration
typedef struct
{
  BYTE Node_ID;             // Current Node ID (1-126)
  BYTE error_code;          // Bits: 0=RxQueue 1=TxQueue 3=CAN
  WORD Baudrate;            // Current Baud rate in kbit
  WORD heartbeat_time;      // Heartbeat time in ms
  WORD heartbeat_timestamp; // Timestamp of last heartbeat
  CAN_MSG heartbeat_msg;    // Heartbeat message contents
  BYTE error_register;      // Error regiter for OD entry [1001,00]
} MCO_CONFIG;

// This structure holds all the TPDO configuration data for one TPDO
typedef struct 
{
#ifdef USE_EVENT_TIME
  WORD event_time;          // Event timer in ms (0 for COS only operation)
  WORD event_timestamp;     // If event timer is used, this is the 
                            // timestamp for the next transmission
#endif
#ifdef USE_INHIBIT_TIME
  WORD inhibit_time;        // Inhibit timer in ms (0 if COS not used)
  WORD inhibit_timestamp;   // If inhibit timer is used, this is the 
                            // timestamp for the next transmission
  BYTE inhibit_status;      // 0: Inhibit timer not started or expired
                            // 1: Inhibit timer started
                            // 2: Transmit msg waiting for expiration of inhibit
#endif
  BYTE offset;              // Offest to application data in process image
  CAN_MSG CAN;              // Current/last CAN message to be transmitted
} TPDO_CONFIG;

// This structure holds all the RPDO configuration data for one RPDO
typedef struct 
{
  WORD CANID;               // Message Identifier 
  BYTE len;                 // Data length (0-8) 
  BYTE offset;              // Pointer to destination of data 
} RPDO_CONFIG;

#ifdef PROCIMG_IN_OD
// This structure holds all data for one process data entry in the OD
typedef struct 
{
  WORD idx;                 // Index of OD entry
  BYTE subidx;              // Subindex of OD entry 
  BYTE len;                 // Data length in bytes (1-4), plus bits ODRD, ODWR, RMAP/WMAP
  BYTE offset;              // Offset to process data in process image
} OD_PROCESS_DATA_ENTRY;
#endif // PROCIMG_IN_OD

/**************************************************************************
GLOBAL FUNCTIONS
**************************************************************************/

/**************************************************************************
DOES:    Initializes the MicroCANopen stack
         It must be called from within MCOUSER_ResetApplication
RETURNS: nothing
**************************************************************************/
void MCO_Init 
  (
  WORD Baudrate,  // CAN baudrate in kbit(1000,800,500,250,125,50,25 or 10)
  BYTE Node_ID,   // CANopen node ID (1-126)
  WORD Heartbeat  // Heartbeat time in ms (0 for none)
  );

/**************************************************************************
DOES:    This function initializes a transmit PDO. Once initialized, the 
         MicroCANopen stack automatically handles transmitting the PDO.
         The application can directly change the data at any time.
NOTE:    For data consistency, the application should not write to the data
         while function MCO_ProcessStack executes.
RETURNS: nothing
**************************************************************************/
void MCO_InitTPDO
  (
  BYTE PDO_NR,       // TPDO number (1-4)
  WORD CAN_ID,       // CAN identifier to be used (set to 0 to use default)
  WORD event_tim,    // Transmitted every event_tim ms 
                     // (set to 0 if ONLY inhibit_tim should be used)
  WORD inhibit_tim,  // Inhibit time in ms for change-of-state transmit
                     // (set to 0 if ONLY event_tim should be used)
  BYTE len,          // Number of data bytes in TPDO
  BYTE offset        // Offset to data location in process image
  );

/**************************************************************************
DOES:    This function initializes a receive PDO. Once initialized, the 
         MicroCANopen stack automatically updates the data at offset.
NOTE:    For data consistency, the application should not read the data
         while function MCO_ProcessStack executes.
RETURNS: nothing
**************************************************************************/
void MCO_InitRPDO
  (
  BYTE PDO_NR,       // RPDO number (1-4)
  WORD CAN_ID,       // CAN identifier to be used (set to 0 to use default)
  BYTE len,          // Number of data bytes in RPDO
  BYTE offset        // Offset to data location in process image
  );

/**************************************************************************
DOES:    This function implements the main MicroCANopen protocol stack. 
         It must be called frequently to ensure proper operation of the
         communication stack. 
         Typically it is called from the while(1) loop in main.
RETURNS: 0 if nothing was done, 1 if a CAN message was sent or received
**************************************************************************/
BYTE MCO_ProcessStack
  (
  void
  );

/**************************************************************************
USER CALL-BACK FUNCTIONS
These must be implemented by the application.
**************************************************************************/

/**************************************************************************
DOES:    Call-back function for reset application.
         Starts the watchdog and waits until watchdog causes a reset.
RETURNS: nothing
**************************************************************************/
void MCOUSER_ResetApplication
  (
  void
  );

/**************************************************************************
DOES:    This function both resets and initializes both the CAN interface
         and the CANopen protocol stack. It is called from within the
         CANopen protocol stack, if a NMT master message was received that
         demanded "Reset Communication".
         This function should call MCO_Init and MCO_InitTPDO/MCO_InitRPDO.
RETURNS: nothing
**************************************************************************/
void MCOUSER_ResetCommunication
  (
  void
  );

/**************************************************************************
DOES:    This function is called if a fatal error occurred. 
         Error codes of mcohwxxx.c are in the range of 0x8000 to 0x87FF.
         Error codes of mco.c are in the range of 0x8800 to 0x8FFF. 
         All other error codes may be used by the application.
RETURNS: nothing
**************************************************************************/
void MCOUSER_FatalError
  (
  WORD ErrCode // To debug, search source code for the ErrCode encountered
  );

#endif
