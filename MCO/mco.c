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

#include "mco.h"
#include "mcohw.h"
#include <string.h>

/**************************************************************************
GLOBAL VARIABLES
***************************************************************************/ 

// this structure holds all node specific configuration
MCO_CONFIG MEM_FAR gMCOConfig;

#if NR_OF_TPDOS > 0
// this structure holds all the TPDO configuration data for up to 4 TPDOs
TPDO_CONFIG MEM_FAR gTPDOConfig[NR_OF_TPDOS];
#endif

// this is the next TPDO to be checked in MCO_ProcessStack
BYTE MEM_FAR gTPDONr = NR_OF_TPDOS;

#if NR_OF_RPDOS > 0
// this structure holds all the RPDO configuration data for up to 4 RPDOs
RPDO_CONFIG MEM_FAR gRPDOConfig[NR_OF_RPDOS];
#endif

// this structure holds the current receive message
CAN_MSG MEM_FAR gRxCAN;

// this structure holds the CAN message for SDO responses or aborts
CAN_MSG MEM_FAR gTxSDO;

// process image from user_xxxx.c
extern BYTE MEM_NEAR gProcImg[];

// table with SDO Responses for read requests to OD - defined in user_xxx.c
extern BYTE MEM_CONST SDOResponseTable[];

#ifdef PROCIMG_IN_OD
// table with Object Dictionary entries to process Data
extern OD_PROCESS_DATA_ENTRY MEM_CONST gODProcTable[];
#endif

/**************************************************************************
LOCAL FUNCTIONS
***************************************************************************/

// SDO Abort Messages
#define SDO_ABORT_UNSUPPORTED     0x06010000UL
#define SDO_ABORT_NOT_EXISTS      0x06020000UL
#define SDO_ABORT_READONLY        0x06010002UL
#define SDO_ABORT_TYPEMISMATCH    0x06070010UL
#define SDO_ABORT_UNKNOWN_COMMAND 0x05040001UL
#define SDO_ABORT_UNKNOWNSUB      0x06090011UL

/**************************************************************************
DOES:    Search the SDO Response table for a specifc index and subindex.
RETURNS: 255 if not found, otherwise the number of the record found
         (staring at zero)
**************************************************************************/
BYTE MCO_Search_OD
  (
  WORD index,   // Index of OD entry searched
  BYTE subindex // Subindex of OD entry searched 
  )
{
  BYTE data i;
  BYTE data i_hi, hi;
  BYTE data i_lo, lo;
  BYTE const code *p;
  BYTE const code *r;

  i = 0;
  i_hi = (BYTE) (index >> 8);
  i_lo = (BYTE) index;
  r = &(SDOResponseTable[0]);
  while (i < 255)
  {
    p = r;
    // set r to next record in table
    r += 8;
    // skip command byte
    p++;
	lo = *p;
	p++;
	hi = *p;
    // if index in table is 0xFFFF, then this is the end of the table
    if ((lo == 0xFF) && (hi == 0xFF))
    {
	  return 255;
	}
    if (lo == i_lo)
    { 
      if (hi == i_hi)
      { 
        p++;
        // entry found?
        if (*p == subindex)
        {
          return i;
        }
      }
    }
    i++;
  }

  // not found
  return 255;
}

#ifdef PROCIMG_IN_OD
/**************************************************************************
DOES:    Search the gODProcTable from user_xxxx.c for a specifc index 
         and subindex.
RETURNS: 255 if not found, otherwise the number of the record found
         (staring at zero)
**************************************************************************/
BYTE MCO_SearchODProcTable
  (
  WORD index,    // Index of OD entry searched
  BYTE subindex  // Subindex of OD entry searched 
  )
{
  BYTE j = 0;
  WORD compare;
  // pointer to od records
  OD_PROCESS_DATA_ENTRY MEM_CONST *pOD;

  // initialize pointer
  pOD = &(gODProcTable[0]);
  // loop until maximum table size
  while (j < 0xFF)
  {
    compare = pOD->idx;
    // end of table reached? 
    if (compare == 0xFFFF)
    {
      return 0xFF;
    }
    // index found?
    if (compare == index)
    {
      // subindex found?
      if (pOD->subidx == subindex)
      {
        return j;
      }
    }
    // increment by SIZEOF(OD_PROCESS_DATA_ENTRY)
    pOD++;
    j++;
  }

  // not found
  return 0xFF;
}

/**************************************************************************
DOES:    Common exit routine for SDO_Handler. 
         Send SDO response with variable length (1-4 bytes).
         Assumes that gTxCAN.ID, LEN and BUF[1-3] are already set
RETURNS: 1 if response transmitted 
**************************************************************************/
BYTE MCO_ReplyWith
  (
  BYTE *pDat,  // pointer to sdo data
  BYTE len     // number of bytes of data in SDO
  )
{
  signed char k; // for loop counter

  // expedited, len of data
  gTxSDO.BUF[0] = 0x43 | ((4-len) << 2);
  // copy "backwards" to swap byte order to lowest first
  for (k = (len - 1); k >= 0; k--)
  {
    gTxSDO.BUF[4+k] = *pDat;
    pDat++;
  }

  // transmit message
  if (!MCOHW_PushMessage(&gTxSDO))
  {
    // failed to transmit
    MCOUSER_FatalError(0x8801);
  }

  // transmitted ok
  return 1;
}
#endif // PROCIMG_IN_OD

/**************************************************************************
DOES:    Generates an SDO Abort Response
RETURNS: nothing
**************************************************************************/
void MCO_Send_SDO_Abort
  (
  DWORD ErrorCode  // 4 byte SDO abort error code
  )
{
  BYTE i;

  // construct message data
  gTxSDO.BUF[0] = 0x80;
  for (i=0;i<4;i++)
  {
    gTxSDO.BUF[4+i] = ErrorCode;
    ErrorCode >>= 8;
  }

  // transmit message
  if (!MCOHW_PushMessage(&gTxSDO))
  {
    // failed to transmit
    MCOUSER_FatalError(0x8801);
  }
}

/**************************************************************************
DOES:    Handle an incoimg SDO request.
RETURNS: returns 1 if SDO access success, returns 0 if SDO abort generated
**************************************************************************/
BYTE MCO_Handle_SDO_Request 
  (
  BYTE *pData  // pointer to 8 data bytes with SDO data
  )
{
  // command byte of SDO request
  BYTE cmd;
  // index of SDO request
  WORD index;
  // subindex of SDO request
  BYTE subindex;
  // search result of Search_OD
  BYTE found;
#ifdef PROCIMG_IN_OD
  BYTE len;
  // pointer to an entry in gODProcTable
  OD_PROCESS_DATA_ENTRY MEM_CONST *pOD;
#endif 

  // init variables
  // upper 3 bits are the command
  cmd = *pData & 0xE0;
  // get high byte of index
  index = pData[2];
  // add low byte of index
  index = (index << 8) + pData[1];
  // subindex
  subindex = pData[3];

  // Copy Multiplexor into response
  // index low
  gTxSDO.BUF[1] = pData[1];
  // index high
  gTxSDO.BUF[2] = pData[2];
  // subindex
  gTxSDO.BUF[3] = pData[3];

  // is it a read or write command?
  if ((cmd == 0x40) || (cmd == 0x20)) 
  {

#ifdef PROCIMG_IN_OD
    // deal with access to process data
    found = MCO_SearchODProcTable(index,subindex);
    // entry found?
    if (found != 0xFF)
    {
	  pOD = &(gODProcTable[found]);
      // read command?
      if (cmd == 0x40)
      {
        // read allowed?
        if ((pOD->len & ODRD) != 0) // Check if RD bit is set
        {
          return MCO_ReplyWith(&(gProcImg[pOD->offset]),(pOD->len & 0x0F));
        }
		// read not allowed
        else
        {
          MCO_Send_SDO_Abort(SDO_ABORT_UNSUPPORTED);
          return 0;
        }
      }
      // write command?
      else
      {
        // is WR bit set? - then write allowed
        if ((pOD->len & ODWR) != 0)
        {
		  // for writes: Bits 2 and 3 of *pData are number of bytes without data
          len = 4 - ((*pData & 0x0C) >> 2); 
          // is length ok?
          if (len != (pOD->len & 0x0F))
          {
            MCO_Send_SDO_Abort(SDO_ABORT_TYPEMISMATCH);
            return 0;
          }
          // retrieve data from SDO write request and copy into process image
          while (len > 0)
          {
            len--;
            gProcImg[pOD->offset+len] = gRxCAN.BUF[4+len];
          }
		  // write response
          gTxSDO.BUF[0] = 0x60;
          if (!MCOHW_PushMessage(&gTxSDO))
          {
            MCOUSER_FatalError(0x8808);
          }
		  return 1;
        }
        // write not allowed
        else
        {
          MCO_Send_SDO_Abort(SDO_ABORT_UNSUPPORTED);
          return 0;
        }
      }
	}
#endif // PROCIMG_IN_OD

    // search const table 
    found = MCO_Search_OD(index,subindex);
    // entry found?
    if (found < 255)
    {
	  // read command?
      if (cmd == 0x40)
      {
        memcpy(&gTxSDO.BUF[0],&SDOResponseTable[(found*8)],8);
        if (!MCOHW_PushMessage(&gTxSDO))
        {
          MCOUSER_FatalError(0x8802);
        }
        return 1;
      }
      // write command
      MCO_Send_SDO_Abort(SDO_ABORT_READONLY);
      return 0;
    }
    if ((index == 0x1001) && (subindex == 0x00))
    {
      // read command
      if (cmd == 0x40)
      {
	    // expedited, 1 byte of data
        gTxSDO.BUF[0] = 0x4F;
        gTxSDO.BUF[4] = gMCOConfig.error_register;
        if (!MCOHW_PushMessage(&gTxSDO))
        {
          MCOUSER_FatalError(0x8802);
        }
        return 1;
      }
      // write command
      MCO_Send_SDO_Abort(SDO_ABORT_READONLY);
      return 0;
    }

#ifdef DYNAMIC_HEARTBEAT
    // hard coding of dynamic read/write accesses
    // access to [1017,00] - heartbeat time
    if ((index == 0x1017) && (subindex == 0x00))
    {
      // read command
      if (cmd == 0x40)
      {
	    // expedited, 2 bytes of data
        gTxSDO.BUF[0] = 0x4B;
        gTxSDO.BUF[4] = (BYTE) gMCOConfig.heartbeat_time;
        gTxSDO.BUF[5] = (BYTE) (gMCOConfig.heartbeat_time >> 8);
        if (!MCOHW_PushMessage(&gTxSDO))
        {
          MCOUSER_FatalError(0x8802);
        }
        return 1;
      }
      // expedited write command with 2 bytes of data
      if (*pData == 0x2B)
      {
        gMCOConfig.heartbeat_time = pData[5];
        gMCOConfig.heartbeat_time = (gMCOConfig.heartbeat_time << 8) + pData[4];
        // write response
        gTxSDO.BUF[0] = 0x60;
		// Needed to pass conformance test: clear unused bytes
        gTxSDO.BUF[4] = 0;
        gTxSDO.BUF[5] = 0;
        gTxSDO.BUF[6] = 0;
        gTxSDO.BUF[7] = 0;
        if (!MCOHW_PushMessage(&gTxSDO))
        {
          MCOUSER_FatalError(0x8802);
        }
        return 1;
      }
      MCO_Send_SDO_Abort(SDO_ABORT_UNSUPPORTED);
      return 0;
    }
#endif // DYNAMIC HEARTBEAT

    // Requested OD entry not found
    if (subindex == 0)
    {
      MCO_Send_SDO_Abort(SDO_ABORT_NOT_EXISTS);
    }
    else
    {
      MCO_Send_SDO_Abort(SDO_ABORT_UNKNOWNSUB);
    }
    return 0;
  }
  // ignore abort received - all other produce an error
  if (cmd != 0x80)
  {
    MCO_Send_SDO_Abort(SDO_ABORT_UNKNOWN_COMMAND);
    return 0;
  }
  return 1;
}


#if NR_OF_TPDOS > 0
/**************************************************************************
DOES:    Called when going into the operational mode.
         Prepares all TPDOs for operational.
RETURNS: nothing
**************************************************************************/
void MCO_Prepare_TPDOs 
  (
    void
  )
{
BYTE i;

  i = 0;
  // prepare all TPDOs for transmission
  while (i < NR_OF_TPDOS)
  {
    // this TPDO is used
    if (gTPDOConfig[i].CAN.ID != 0)
    {
      // Copy current process data
      memcpy(&gTPDOConfig[i].CAN.BUF[0],&(gProcImg[gTPDOConfig[i].offset]),gTPDOConfig[i].CAN.LEN);
#ifdef USE_EVENT_TIME
      // Reset event timer for immediate transmission
      gTPDOConfig[i].event_timestamp = MCOHW_GetTime() - 2;
#endif
#ifdef USE_INHIBIT_TIME
      gTPDOConfig[i].inhibit_status = 2; // Mark as ready for transmission
      // Reset inhibit timer for immediate transmission
      gTPDOConfig[i].inhibit_timestamp = MCOHW_GetTime() - 2;
#endif
    }
  i++;
  }
  // ensure that MCO_ProcessStack starts with TPDO1
  gTPDONr = NR_OF_TPDOS;
}

/**************************************************************************
DOES:    Called when a TPDO needs to be transmitted
RETURNS: nothing
**************************************************************************/
void MCO_TransmitPDO 
  (
  BYTE PDONr  // TPDO number to transmit
  )
{
#ifdef USE_INHIBIT_TIME
  // new inhibit timer started
  gTPDOConfig[PDONr].inhibit_status = 1;
  gTPDOConfig[PDONr].inhibit_timestamp = MCOHW_GetTime() + gTPDOConfig[PDONr].inhibit_time;
#endif
#ifdef USE_EVENT_TIME
  gTPDOConfig[gTPDONr].event_timestamp = MCOHW_GetTime() + gTPDOConfig[gTPDONr].event_time; 
#endif
  if (!MCOHW_PushMessage(&gTPDOConfig[PDONr].CAN))
  {
    MCOUSER_FatalError(0x8801);
  }
}
#endif // NR_OF_TPDOS > 0

/**************************************************************************
PUBLIC FUNCTIONS
***************************************************************************/ 

/**************************************************************************
DOES:    Initializes the MicroCANopen stack
         It must be called from within MCOUSER_ResetApplication
RETURNS: nothing
**************************************************************************/
void MCO_Init 
  (
  WORD Baudrate,  // CAN baudrate in kbit (1000,800,500,250,125,50,25 or 10)
  BYTE Node_ID,   // CANopen node ID (1-126)
  WORD Heartbeat  // Heartbeat time in ms (0 for none)
  )
{
  BYTE i;

  // Init the global variables
  gMCOConfig.Node_ID = Node_ID;
  gMCOConfig.error_code = 0;
  gMCOConfig.Baudrate = Baudrate;
  gMCOConfig.heartbeat_time = Heartbeat;
  gMCOConfig.heartbeat_msg.ID = 0x700+Node_ID;
  gMCOConfig.heartbeat_msg.LEN = 1;
  // current NMT state of this node = bootup
  gMCOConfig.heartbeat_msg.BUF[0] = 0;
  gMCOConfig.error_register = 0;
 
  // Init SDO Response/Abort message
  gTxSDO.ID = 0x580+gMCOConfig.Node_ID;
  gTxSDO.LEN = 8;
   
#if NR_OF_TPDOS > 0
  i = 0;
  // init TPDOs
  while (i < NR_OF_TPDOS)
  {
    gTPDOConfig[i].CAN.ID = 0;
    i++;
  }
#endif
#if NR_OF_RPDOS > 0
  i = 0;
  // init RPDOs
  while (i < NR_OF_RPDOS)
  {
    gRPDOConfig[i].CANID = 0;
    i++;
  }
#endif

  // init the CAN interface
  if (!MCOHW_Init(Baudrate))
  {
    MCOUSER_FatalError(0x8802);
  }
  // for nmt master message
  if (!MCOHW_SetCANFilter(0))
  {
    MCOUSER_FatalError(0x8803);
  }
  // for SDO requests
  if (!MCOHW_SetCANFilter(0x600+Node_ID))
  {
    MCOUSER_FatalError(0x8803);
  }

  // signal to MCO_ProcessStack: we just initialized
  gTPDONr = 0xFF;
}  

#if NR_OF_RPDOS > 0
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
  )
{

#ifdef CHECK_PARAMETERS
  // check PDO range and check node id range 1 - 127
  if (((PDO_NR < 1)             || (PDO_NR > NR_OF_RPDOS))      || 
      ((gMCOConfig.Node_ID < 1) || (gMCOConfig.Node_ID > 127)))
  {
    MCOUSER_FatalError(0x8804);
  }
  // is size of process image exceeded?
  if (offset >= PROCIMG_SIZE)   
  { 
    MCOUSER_FatalError(0x8904);
  }
#endif
  PDO_NR--;
  gRPDOConfig[PDO_NR].len = len;
  gRPDOConfig[PDO_NR].offset = offset;
  if (CAN_ID == 0)
  {
    gRPDOConfig[PDO_NR].CANID = 0x200 + (0x100 * ((WORD)(PDO_NR))) + gMCOConfig.Node_ID;
  }
  else
  {
    gRPDOConfig[PDO_NR].CANID = CAN_ID;
  }
  if (!MCOHW_SetCANFilter(gRPDOConfig[PDO_NR].CANID))
  {
    MCOUSER_FatalError(0x8805);
  }
}
#endif // NR_OF_RPDOS > 0


#if NR_OF_TPDOS > 0
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
  BYTE PDO_NR,        // TPDO number (1-4)
  WORD CAN_ID,        // CAN identifier to be used (set to 0 to use default)
  WORD event_time,    // Transmitted every event_tim ms 
  WORD inhibit_time,  // Inhibit time in ms for change-of-state transmit
                      // (set to 0 if ONLY event_tim should be used)
  BYTE len,           // Number of data bytes in TPDO
  BYTE offset         // Offset to data location in process image
  )
{

#ifdef CHECK_PARAMETERS
  // check PDO range, node id, len range 1 - 8 and event time or inhibit time set
  if (((PDO_NR < 1)             || (PDO_NR > NR_OF_TPDOS))     ||
      ((gMCOConfig.Node_ID < 1) || (gMCOConfig.Node_ID > 127)) ||
      ((len < 1)                || (len > 8))                  ||
      ((event_time == 0)        && (inhibit_time == 0)))
  {
    MCOUSER_FatalError(0x8806);
  }
  // is size of process image exceeded?
  if (offset >= PROCIMG_SIZE)   
  { 
    MCOUSER_FatalError(0x8906);
  }
#endif
  PDO_NR--;
  if (CAN_ID == 0)
  {
    gTPDOConfig[PDO_NR].CAN.ID = 0x180 + (0x100 * ((WORD)(PDO_NR))) + gMCOConfig.Node_ID;
  }
  else
  {
    gTPDOConfig[PDO_NR].CAN.ID = CAN_ID;
  }
  gTPDOConfig[PDO_NR].CAN.LEN = len;
  gTPDOConfig[PDO_NR].offset = offset;
#ifdef USE_EVENT_TIME
  gTPDOConfig[PDO_NR].event_time = event_time;
#endif
#ifdef USE_INHIBIT_TIME
  gTPDOConfig[PDO_NR].inhibit_time = inhibit_time;
#endif
}
#endif // NR_OF_TPDOS > 0


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
  )
{
  BYTE i;
  BYTE ret_val = 0;

  // check if this is right after boot-up
  // was set by MCO_Init
  if (gTPDONr == 0xFF)
  {
    // init heartbeat time
    gMCOConfig.heartbeat_timestamp = MCOHW_GetTime() + gMCOConfig.heartbeat_time;
    // send boot-up message  
    if (!MCOHW_PushMessage(&gMCOConfig.heartbeat_msg))
    {
      MCOUSER_FatalError(0x8801);
    }
#ifdef AUTOSTART
    // going into operational state
    gMCOConfig.heartbeat_msg.BUF[0] = 0x05;
#if NR_OF_TPDOS > 0
    MCO_Prepare_TPDOs();
#endif
#else
    // going into pre-operational state
    gMCOConfig.heartbeat_msg.BUF[0] = 0x7F;
#endif
    // return value to default
    gTPDONr = NR_OF_TPDOS;
    return 1;
  }
 
  // work on next incoming messages
  // if message received
  if (MCOHW_PullMessage(&gRxCAN))
  {
    // is it an NMT master message?
    if (gRxCAN.ID == 0)
    {
      // nmt message is for this node or all nodes
      if ((gRxCAN.BUF[1] == gMCOConfig.Node_ID) || (gRxCAN.BUF[1] == 0))
      {
        switch (gRxCAN.BUF[0])
        {
          // start node
          case 1:
            gMCOConfig.heartbeat_msg.BUF[0] = 5;
#if NR_OF_TPDOS > 0          
            MCO_Prepare_TPDOs();
#endif
            break;

          // stop node
          case 2:
            gMCOConfig.heartbeat_msg.BUF[0] = 4;
            break;

          // enter pre-operational
          case 128:
            gMCOConfig.heartbeat_msg.BUF[0] = 127;
            break;
   
          // node reset
          case 129:
            MCOUSER_ResetApplication();
            break;

          // node reset communication
          case 130:
            MCOUSER_ResetCommunication();
            break;

          // unknown command
          default:
            break;
        }

        return 1;
      } // NMT message addressed to this node
    } // NMT master message received
    
    // if node is not stopped...
    if (gMCOConfig.heartbeat_msg.BUF[0] != 4)
    {
      // is the message an SDO request message for us?
      if (gRxCAN.ID == gMCOConfig.Node_ID+0x600)
      {
        // handle SDO request - return value not used in this version
        i = MCO_Handle_SDO_Request(&gRxCAN.BUF[0]);
        return 1;
      }
    }

#if NR_OF_RPDOS > 0
    // is the node operational?
    if (gMCOConfig.heartbeat_msg.BUF[0] == 5)
    {
      i = 0;
      // loop through RPDOs
      while (i < NR_OF_RPDOS)
      {
        // is this one of our RPDOs?
        if (gRxCAN.ID == gRPDOConfig[i].CANID)
        {
          // copy data from RPDO to process image
          memcpy(&(gProcImg[gRPDOConfig[i].offset]),&(gRxCAN.BUF[0]),gRPDOConfig[i].len);
          // exit the loop
          i = NR_OF_RPDOS;
          ret_val = 1;
        }
        i++;
      } // for all RPDOs
    } // node is operational
#endif // NR_OF_RPDOS > 0
  } // Message received

#if NR_OF_TPDOS > 0
  // is the node operational?
  if (gMCOConfig.heartbeat_msg.BUF[0] == 5)
  {
    // check next TPDO for transmission
    gTPDONr++;
    if (gTPDONr >= NR_OF_TPDOS)
    {
      gTPDONr = 0;
    }
    // is the TPDO 'gTPDONr' in use?
    if (gTPDOConfig[gTPDONr].CAN.ID != 0)
    {
#ifdef USE_EVENT_TIME
      // does TPDO use event timer and event timer is expired? if so we need to transmit now
      if ((gTPDOConfig[gTPDONr].event_time != 0) && 
          (MCOHW_IsTimeExpired(gTPDOConfig[gTPDONr].event_timestamp)) )
      {
        // get data from process image and transmit
        memcpy(&(gTPDOConfig[gTPDONr].CAN.BUF[0]),&(gProcImg[gTPDOConfig[gTPDONr].offset]),gTPDOConfig[gTPDONr].CAN.LEN);
        MCO_TransmitPDO(gTPDONr);
        return 1;
      }
#endif // USE_EVENT_TIME
#ifdef USE_INHIBIT_TIME
      // does the TPDO use an inhibit time? - COS transmission
      if (gTPDOConfig[gTPDONr].inhibit_time != 0)
      {
        // is the inihibit timer currently running?
        if (gTPDOConfig[gTPDONr].inhibit_status > 0)
        {
          // has the inhibit time expired?
          if (MCOHW_IsTimeExpired(gTPDOConfig[gTPDONr].inhibit_timestamp))
          {
            // is there a new transmit message already waiting?
		    if (gTPDOConfig[gTPDONr].inhibit_status == 2)
			{ 
              // transmit now
              MCO_TransmitPDO(gTPDONr);
              return 1;
            }
          }
          // no new message waiting, but timer expired
          else 
		  {
		    gTPDOConfig[gTPDONr].inhibit_status = 0;
		  }
		}
        // is inhibit status 0 or 1?
        if (gTPDOConfig[gTPDONr].inhibit_status < 2)
        {
          // has application data changed?
          if ((memcmp(&gTPDOConfig[gTPDONr].CAN.BUF[0],&(gProcImg[gTPDOConfig[gTPDONr].offset]),gTPDOConfig[gTPDONr].CAN.LEN) != 0))
          {
            // Copy application data
            memcpy(&gTPDOConfig[gTPDONr].CAN.BUF[0],&(gProcImg[gTPDOConfig[gTPDONr].offset]),gTPDOConfig[gTPDONr].CAN.LEN);
            // has inhibit time expired?
            if (gTPDOConfig[gTPDONr].inhibit_status == 0)
            {
              // transmit now
              MCO_TransmitPDO(gTPDONr);
              return 1;
            }
            // inhibit status is 1
 			else
            {
              // wait for inhibit time to expire 
              gTPDOConfig[gTPDONr].inhibit_status = 2;
			}
          }
        }
      } // Inhibit Time != 0
#endif // USE_INHIBIT_TIME
    } // PDO active (CAN_ID != 0)  
  } // if node is operational
#endif // NR_OF_TPDOS > 0
  
  // do we produce a heartbeat?
  if (gMCOConfig.heartbeat_time != 0)
  {
    // has heartbeat time passed?
    if (MCOHW_IsTimeExpired(gMCOConfig.heartbeat_timestamp))
    {
      // transmit heartbeat message
      if (!MCOHW_PushMessage(&gMCOConfig.heartbeat_msg))
      {
        MCOUSER_FatalError(0x8801);
      }
      // get new heartbeat time for next transmission
      gMCOConfig.heartbeat_timestamp = MCOHW_GetTime() + gMCOConfig.heartbeat_time;
      ret_val = 1;
    }
  }
  return ret_val;
}

