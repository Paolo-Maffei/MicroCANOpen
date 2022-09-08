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
MODULE:    USER
CONTAINS:  MicroCANopen Object Dictionary and Process Image implementation
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
VERSION:   1.21, Pf/Aa/Ck 16-OCT-03
---------------------------------------------------------------------------
HISTORY:   1.21, AA, 16-OCT-03, Cleanup and minor corrections
           1.20, Pf 19-AUG-03, First Published Version
           Functionality in this module was located in module MCO in
	       previous releases.
***************************************************************************/ 

#include "../mco/mco.h"
#include "r591io.h"
#include "string.h"
#include <reg591.h>

// ensure the number of tpdos and rpdos is correct
#if (NR_OF_RPDOS != 2)
  #if (NR_OF_TPDOS != 2)
#error This example is for 2 TPDOs and 2 RPDOs only
  #endif
#endif

// MACROS FOR OBJECT DICTIONARY ENTRIES
#define GETBYTE(val,pos) ((val >> pos) & 0xFF)
#define GETBYTES16(val) GETBYTE(val, 0), GETBYTE(val, 8)
#define GETBYTES32(val) GETBYTE(val, 0), GETBYTE(val, 8), GETBYTE(val,16), GETBYTE(val,24)
#define SDOREPLY(index,sub,len,val) 0x43 | ((4-len)<<2), GETBYTES16(index), sub, GETBYTES32(val)
#define SDOREPLY4(index,sub,len,d1,d2,d3,d4) 0x43 | ((4-len)<<2), GETBYTES16(index), sub, d1, d2, d3, d4

#define ODENTRY(index,sub,len,offset) {index, sub, len, offset}

// global variables

// This structure holds all node specific configuration
BYTE MEM_NEAR gProcImg[PROCIMG_SIZE];

// Table with SDO Responses for read requests to OD
// Each Row has 8 Bytes:
// Command Specifier for SDO Response (1 byte)
//   bits 2+3 contain: '4' – {number of data bytes}
// Object Dictionary Index (2 bytes, low first)
// Object Dictionary Subindex (1 byte)
// Data (4 bytes, lowest bytes first)
BYTE MEM_CONST SDOResponseTable[] = {

  // [1000h,00]: Device Type
  SDOREPLY(0x1000, 0x00, 4, OD_DEVICE_TYPE),

#ifdef OD_SERIAL
  // [1018h,00]: Identity Object, Number of Entries = 4
  SDOREPLY(0x1018, 0x00, 1, 0x00000004L),
#else
  // [1018h,00]: Identity Object, Number of Entries = 3
  SDOREPLY(0x1018, 0x00, 1, 0x00000003L),
#endif

  // [1018h,01]: Identity Object, Vendor ID
  SDOREPLY(0x1018, 0x01, 4, OD_VENDOR_ID),

  // [1018h,02]: Identity Object, Product Code
  SDOREPLY(0x1018, 0x02, 4, OD_PRODUCT_CODE),

  // [1018h,03]: Identity Object, Revision
  SDOREPLY(0x1018, 0x03, 4, OD_REVISION),

#ifdef OD_SERIAL
  // [1018h,04]: Identity Object, Serial
  SDOREPLY(0x1018, 0x04, 4, OD_SERIAL),
#endif

  // [2018h,00]: MicroCANopen Identity Object, Number of Entries = 3
  SDOREPLY(0x2018, 0x00, 1, 0x00000003L),

  // [2018h,01]: MicroCANopen Identity Object, Vendor ID = 01455341, ESA Inc.
  SDOREPLY(0x2018, 0x01, 4, 0x01455341L),

  // [2018h,02]: MicroCANopen Identity Object, Product Code = "MCOP"
  SDOREPLY4(0x2018, 0x02, 4, 'P', 'O', 'C', 'M'),

  // [2018h,03]: MicroCANopen Identity Object, Revision = 1.20
  SDOREPLY(0x2018, 0x03, 4, 0x00010020L),

#ifdef PDO_IN_OD
  // NOTE: These entries must be added manually. The parameters must match
  // the parameters used to call the functions MCO_InitRPDO and MCO_InitTPDO.

  // RPDO1 Communication Parameters
  SDOREPLY(0x1400, 0x00, 1, 0x00000002L),
  SDOREPLY(0x1400, 0x01, 4, 0x00000207L),
  SDOREPLY(0x1400, 0x02, 1, 0x000000FFL),

  // RPDO2 Communication Parameters
  SDOREPLY(0x1401, 0x00, 1, 0x00000002L),
  SDOREPLY(0x1401, 0x01, 4, 0x00000200L),
  SDOREPLY(0x1401, 0x02, 1, 0x000000FFL),

  // RPDO1 Mapping Parameters
  SDOREPLY(0x1600, 0x00, 1, 0x00000004L),
  SDOREPLY(0x1600, 0x01, 4, 0x62000108L),
  SDOREPLY(0x1600, 0x02, 4, 0x62000208L),
  SDOREPLY(0x1600, 0x03, 4, 0x62000308L),
  SDOREPLY(0x1600, 0x04, 4, 0x62000408L),

  // RPDO2 Mapping Parameters
  SDOREPLY(0x1601, 0x00, 1, 0x00000002L),
  SDOREPLY(0x1601, 0x01, 4, 0x64110110L),
  SDOREPLY(0x1601, 0x02, 4, 0x64110210L),

  // TPDO1 Communication Parameters
  SDOREPLY(0x1800, 0x00, 1, 0x00000002L),
  SDOREPLY(0x1800, 0x01, 4, 0x40000187L),
  SDOREPLY(0x1800, 0x02, 1, 0x000000FFL),
  SDOREPLY(0x1800, 0x03, 2, 0L),
  SDOREPLY(0x1800, 0x05, 2, 100L),

  // TPDO2 Communication Parameters
  SDOREPLY(0x1801, 0x00, 1, 0x00000002L),
  SDOREPLY(0x1801, 0x01, 4, 0x40000287L),
  SDOREPLY(0x1801, 0x02, 1, 0x000000FFL),
  SDOREPLY(0x1801, 0x03, 2, 50L),
  SDOREPLY(0x1801, 0x05, 2, 100L),

  // TPDO1 Mapping Parameters
  SDOREPLY(0x1A00, 0x00, 1, 0x00000004L),
  SDOREPLY(0x1A00, 0x01, 4, 0x60000108L),
  SDOREPLY(0x1A00, 0x02, 4, 0x60000208L),
  SDOREPLY(0x1A00, 0x03, 4, 0x60000308L),
  SDOREPLY(0x1A00, 0x04, 4, 0x60000408L),

  // TPDO2 Mapping Parameters
  SDOREPLY(0x1A01, 0x00, 1, 0x00000002L),
  SDOREPLY(0x1A01, 0x01, 4, 0x64010110L),
  SDOREPLY(0x1A01, 0x02, 4, 0x64010210L),

  // Nr of Entries for subindex zero of arrays used in gODProcTable
  SDOREPLY(0x6000, 0x00, 1, 0x00000004L),
  SDOREPLY(0x6200, 0x00, 1, 0x00000004L),
  SDOREPLY(0x6401, 0x00, 1, 0x00000002L),
  SDOREPLY(0x6411, 0x00, 1, 0x00000002L),
#endif // PDO_IN_OD

  // End-of-table marker
  SDOREPLY(0xFFFF, 0xFF, 0xFF, 0xFFFFFFFFL)
};

#ifdef PROCIMG_IN_OD
// Table with Object Dictionary entries to process Data
OD_PROCESS_DATA_ENTRY MEM_CONST gODProcTable[] = {

  // IN_digi
  ODENTRY(0x6000,0x01,1+ODRD,IN_digi_1),
  ODENTRY(0x6000,0x02,1+ODRD,IN_digi_2),
  ODENTRY(0x6000,0x03,1+ODRD,IN_digi_3),
  ODENTRY(0x6000,0x04,1+ODRD,IN_digi_4),

  // IN_ana
  ODENTRY(0x6401,0x01,2+ODRD,IN_ana_1),
  ODENTRY(0x6401,0x02,2+ODRD,IN_ana_2),

  // OUT_digi
  ODENTRY(0x6200,0x01,1+ODRD+ODWR,OUT_digi_1),
  ODENTRY(0x6200,0x02,1+ODRD+ODWR,OUT_digi_2),
  ODENTRY(0x6200,0x03,1+ODRD+ODWR,OUT_digi_3),
  ODENTRY(0x6200,0x04,1+ODRD+ODWR,OUT_digi_4),

  // OUT_ana
  ODENTRY(0x6411,0x01,2+ODRD+ODWR,OUT_ana_1),
  ODENTRY(0x6411,0x02,2+ODRD+ODWR,OUT_ana_2),

  // End-of-table marker
  ODENTRY(0xFFFF,0xFF,0xFF,0xFF)
};
#endif // PROCIMG_IN_OD

/**************************************************************************
DOES:    This function is called if a fatal error occurred. 
         Error codes of mcohwxxx.c are in the range of 0x8000 to 0x87FF.
         Error codes of mco.c are in the range of 0x8800 to 0x8FFF. 
         All other error codes may be used by the application.
RETURNS: nothing
**************************************************************************/
void MCOUSER_FatalError
  (
  WORD ErrCode  // the error code
  )
{
  //display blinking pattern on led
  error_state(ErrCode & 0xFF);
}

/**************************************************************************
DOES:    Call-back function for reset application.
         Starts the watchdog and waits until watchdog causes a reset.
RETURNS: nothing
**************************************************************************/
void MCOUSER_ResetApplication
  (
  void
  )
{
  MCOUSER_ResetCommunication();
}

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
  )
{
  BYTE i;

  EA = 0;

  // Initialize Process Variables
  for (i = 0; i < PROCIMG_SIZE; i++)
  {
    gProcImg[i] = 0;
  }

  // 125kbit, Node 7, 2s heartbeat
  MCO_Init(125,0x07,2000); 
  
  // RPDO1, default ID (0x200+nodeID), 4 bytes
  MCO_InitRPDO(1,0,4,OUT_digi_1); 

  // RPDO2, default ID (0x300+nodeID), 4 bytes
  MCO_InitRPDO(2,0,4,OUT_ana_1); 

  // TPDO1, default ID (0x180+nodeID), 100ms event, 0ms inhibit, 4 bytes
  // Transmit trigger: COS (change-of-state) with 250ms inhibit time
  MCO_InitTPDO(1,0,100,0,4,IN_digi_1);    
  
  // TPDO2, default ID (0x280+nodeID), 100ms event, 5ms inhibit, 4 bytes
  // Transmit trigger: COS with 100ms inhibit time, but if there is no COS within
  // 1000ms, message gets triggered, too
  MCO_InitTPDO(2,0,100,5,4,IN_ana_1); 
}

