/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: qspim_drv.c
 *      Project: QSPIM driver (MDIS4)
 *
 *       Author: kp
 *        $Date: 2015/02/19 11:56:46 $
 *    $Revision: 2.7 $
 *
 *  Description: Low-level driver for QSPI interface Mahr project
 *
 *     Required: OSS, DESC, DBG, ID libraries
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *		 QSPIM_SUPPORT_8240_DMA - use 8240 DMA. Works only on 8240 Map B boards
 *       QSPIM_SUPPORT_A21_DMA  - support special customerspecific DMA mode of A21
 *		 QSPIM_D201_SW 			- D201 in swapped mode
 *
 *	   Specification: 13Z010-06_S4.doc
 *
 *-------------------------------[ History ]---------------------------------
 *
 *  --- end of mcvs controlled history log. see Stash for newer commits. ---
 * 
 * $Log: qspim_drv.c,v $
 * Revision 2.7  2015/02/19 11:56:46  ts
 * R: changes in QSPI core on customer request
 * M: built in direct QSPI mode, auto mode, DMA transfer
 *
 * Revision 2.6  2014/08/26 13:53:56  jt
 * R: It was not possible to use blocking I/O on driver's read
 * M: Implement blocking read functionallity
 *
 * Revision 2.5  2012/03/12 13:55:03  dpfeuffer
 * R:1. QSPIM_BC02 example application for AE57
 * M:1.a) QSPIM_NO_DUPLICATION setstat added
 *     b) QSPIM_Irq(): added option to prevent frame duplication
 *
 * Revision 2.4  2010/05/06 11:06:20  amorbach
 * R: 1.  Porting to MDIS5 (according porting guide rev. 0.7)
 * 2.  EaI: setstat value can be zero - division by zero
 * M: 1a. added support for 64bit (Set/GetStat prototypes, m_read calls)
 * 1b. put all MACCESS macros conditionals in brackets
 * 2.  parameter check added
 *
 * Revision 2.3  2010/04/30 15:08:04  ag
 * R:1. Driver didn’t work on little-endian platforms.
 * M:1. Adapted register layout because swapping was removed in FPGA.
 * ATTENTION: For EM1A now driver_z76_em1a(_sw).mak must be used.
 *
 * Revision 2.2  2006/03/01 20:49:14  cs
 * replaced OSS_IrqMask/OSS_IrqUnMask with OSS_IrqMaskR/OSS_IrqRestore
 * removed timing debugs (getTimeBase())
 *     (not support by all OS as it was implemented)
 *
 * Revision 2.1  2001/05/25 11:09:20  kp
 * Added support to call special setstat routines from user interrupt service
 * routines directly. Currently implemented: QSPIM_FRAMESYN
 *
 * Revision 2.0  2001/04/11 10:22:52  kp
 * Major changes to run on A12, using 8240 DMA
 *
 * Revision 1.1  2000/09/25 13:24:07  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include "qspim_int.h"

#if defined(LINUX) && defined(QSPIM_SUPPORT_A21_DMA)
#include <linux/slab.h>
#endif

/* #define GPIO_DEBUG 1 */

#ifndef _MAC_OFF_
#define _MAC_OFF_ 	0
#endif


#define QSPISWAP(x) ((((x) & 0xff000000) >> 8)  |	\
				 (((x) & 0x00ff0000) << 8)  |	\
				 (((x) & 0x0000ff00) >> 8)  |	\
				 (((x) & 0x000000ff) << 8))


/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 QSPIM_Init(DESC_SPEC *descSpec, 
			OSS_HANDLE *osHdl,
			MACCESS ma[ADDRSPACE_COUNT], 
			OSS_SEM_HANDLE *devSemHdl,
			OSS_IRQ_HANDLE *irqHdl, 
			LL_HANDLE **hP);
static int32 QSPIM_Exit(LL_HANDLE **hP );
static int32 QSPIM_Read(LL_HANDLE *h, int32 ch, int32 *value);
static int32 QSPIM_Write(LL_HANDLE *h, int32 ch, int32 value);
static int32 QSPIM_SetStat(LL_HANDLE *h,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 QSPIM_GetStat(LL_HANDLE *h, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 QSPIM_BlockRead(LL_HANDLE *h, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 DirectWriteFunc(
	void *arg,
	u_int16 *qspiFrame,
	u_int32 qspiFrameLen);
static int32 DirectISetstat(
	void   *arg,
    int32  code,
    int32  ch,
    int32  value);
static int32 QSPIM_BlockWrite(LL_HANDLE *h, int32 ch, void *buf, int32 size,
							 int32 *nbrWrBytesP);
static int32 QSPIM_Irq(LL_HANDLE *h );
static int32 QSPIM_Info(int32 infoType, ... );

static char* Ident( void );
static int32 Cleanup(LL_HANDLE *h, int32 retCode);
static void FreeRcvFifo( LL_HANDLE *h );
static int32 CreateRcvFifo( LL_HANDLE *h, int32 depth );
static void ClrRcvFifo( LL_HANDLE *h );
static int32 DefineFrm( LL_HANDLE *h, M_SG_BLOCK *blk );
static void CopyBlock( u_int32 *src, u_int32 *dst, u_int32 size );

/**************************** QSPIM_GetEntry *********************************
 *
 *  Description:  Initialize driver's jump table
 *
 *---------------------------------------------------------------------------
 *  Input......:  ---
 *  Output.....:  drvP  pointer to the initialized jump table structure
 *  Globals....:  ---
 ****************************************************************************/
#ifdef _ONE_NAMESPACE_PER_DRIVER_
    extern void LL_GetEntry( LL_ENTRY* drvP )
#else
    extern void __QSPIM_GetEntry( LL_ENTRY* drvP )
#endif
{
    drvP->init        = QSPIM_Init;
    drvP->exit        = QSPIM_Exit;
    drvP->read        = QSPIM_Read;
    drvP->write       = QSPIM_Write;
    drvP->blockRead   = QSPIM_BlockRead;
    drvP->blockWrite  = QSPIM_BlockWrite;
    drvP->setStat     = QSPIM_SetStat;
    drvP->getStat     = QSPIM_GetStat;
    drvP->irq         = QSPIM_Irq;
    drvP->info        = QSPIM_Info;
}

/******************************** QSPIM_Init *********************************
 *
 *  Description:  Allocate and return low-level handle, initialize hardware
 *
 *  The function
 *  - scans the descriptor
 *	- loads the loadable PLD
 *  - initializes the registers
 *
 *  After QSPM_Init the timer/QSPI is stopped and must be started with
 *  TIMER_STATE setstat
 *
 *  Descriptor key        Default          Range/Unit
 *  --------------------  ---------------  -------------
 *  DEBUG_LEVEL_DESC      OSS_DBG_DEFAULT  see dbg.h
 *	DEBUG_LEVEL           OSS_DBG_DEFAULT  see dbg.h
 *  PLD_LOAD		 	  1           	   0..1
 *  PLD_CLOCK			  33333333		   [Hz]
 *  QSPI_QUEUE_LEN		  256			   16..256 [words]
 *  WOMQ				  0				   0/1
 *	BITS				  16			   8..16
 *	CPOL				  0				   0/1
 *	CPHA				  0				   0/1
 *	BAUD				  4000000		   [Hz]
 *	DSCLK				  2000			   [ns]
 *	DTL					  1000			   [ns]
 *	PCS_DEFSTATE		  0				   0x0..0xf
 *	TIMER_CYCLE_TIME	  500000		   [ns]
 *	RCV_FIFO_DEPTH		  10			   2..n
 *
 *  PLD_LOAD: determines if PLD will be loaded
 *  PLD_CLOCK: clock rate of PLD in Hz
 *  QSPI_QUEUE_LEN: 64 for D201 prototype, 256 for A12
 *
 *  other codes: see corresponding setstats
 *---------------------------------------------------------------------------
 *  Input......:  descSpec   pointer to descriptor data
 *                osHdl      oss handle
 *                ma         D201 variant: 	ma[0] hw access handle for Plx
 *							 				ma[1] hw access handle for QSPI
 *				  ma		 STD variant:   ma[0] hw access handle for QSPI
 *                devSemHdl  device semaphore handle
 *                irqHdl     irq handle
 *  Output.....:  hP     	 pointer to low-level driver handle
 *                return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
	MACCESS         ma[],
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **hP
)
{
    LL_HANDLE *h = NULL;
    u_int32 gotsize;
    int32 error, val;
    u_int32 value;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
	*hP = NULL;		/* set low-level driver handle to NULL */

	/* alloc */
    if ((h = (LL_HANDLE*)OSS_MemGet(
    				osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	/* clear */
    OSS_MemFill(osHdl, gotsize, (char*)h, 0x00);

	/* init */
    h->memAlloc   = gotsize;
    h->osHdl      = osHdl;
    h->irqHdl     = irqHdl;
#ifdef QSPIM_D201_SW
    h->maPlx      = ma[0];
    h->maQspi     = ma[1];
#elif defined (QSPIM_SUPPORT_A21_DMA) || defined (QSPIM_SUPPORT_8240_DMA)
    /* this FPGA on A21 uses a group of 3 IP cores to form DMA support  */
    h->maQspi     = ma[0];
    h->maDMA      = ma[1];
    h->maSRAM     = ma[2];
#else
    h->maQspi     = ma[0];
#endif
	
    DBGWRT_1((DBH, "LL - QSPIM_Init: MACCESS handles:\n"));
    DBGWRT_1((DBH, " ma[0] (maQspi) = %08x\n", h->maQspi ));
# if defined (QSPIM_SUPPORT_A21_DMA) || defined (QSPIM_SUPPORT_8240_DMA)
    DBGWRT_1((DBH, " ma[1] (maDma)  = %08x\n", h->maDMA  ));
    DBGWRT_1((DBH, " ma[2] (maSRAM) = %08x\n", h->maSRAM ));
# endif
    
    h->qpdrShadow = 0;
    h->devSemHdl = devSemHdl;
    
    if ((error = OSS_SemCreate(h->osHdl, OSS_SEM_BIN, 0,
							   &h->readSemHdl))) {
      OSS_MemFree(osHdl, h, gotsize); /* klocwork 2nd id20012 */
      return error;
    }

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
	/* driver's ident function */
	h->idFuncTbl.idCall[0].identCall = Ident;
	/* library's ident functions */
	h->idFuncTbl.idCall[1].identCall = DESC_Ident;
	h->idFuncTbl.idCall[2].identCall = OSS_Ident;
#ifdef QSPIM_D201_SW
	h->idFuncTbl.idCall[3].identCall = PLD_FLEX10K_Identify;
	h->idFuncTbl.idCall[4].identCall = __QSPIM_PldIdent;
	/* terminator */
	h->idFuncTbl.idCall[5].identCall = NULL;
#else
	/* terminator */
	h->idFuncTbl.idCall[3].identCall = NULL;
#endif
    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
	DBGINIT((NULL,&DBH));

    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
	/* prepare access */
    if ((error = DESC_Init(descP, osHdl, &h->descHdl)))
		return( Cleanup(h,error) );

	GET_PARAM( "DEBUG_LEVEL_DESC", OSS_DBG_DEFAULT, value );
	DESC_DbgLevelSet(h->descHdl, value);	/* set level */

	GET_PARAM( "DEBUG_LEVEL", OSS_DBG_DEFAULT, h->dbgLevel );

    DBGWRT_1((DBH, "LL - QSPIM_Init\n"));

	GET_PARAM( "PLD_LOAD", TRUE, h->pldLoad );
	GET_PARAM( "PLD_CLOCK", 33333333, h->pldClock );
	GET_PARAM( "QSPI_QUEUE_LEN", 256, h->qspiQueueLen );

	/*------------------------------+
	|  Load PLD                     |
	+------------------------------*/
#ifdef QSPIM_D201_SW
	if( h->pldLoad ){
		DBGWRT_2((DBH," loading PLD\n"));
		error = __LoadD201Pld(h);

		if( error )
			return Cleanup(h, error );
	}
#endif
    /*------------------------------+
    |  init QSPI                    |
    +------------------------------*/
	MWRITE_D16( h->maQspi, QSPI_SPCR1, 0x0000 ); 	/* stops QSPI */
	MWRITE_D8( h->maQspi, QSPI_SPSR, 0x00 ); 		/* clear SPIF */
	MWRITE_D16( h->maQspi, QSPI_SPCR2, 0x0000 ); 	/* disable QSPI irqs */
	MWRITE_D16( h->maQspi, QSPI_SPCR0, 0x8000 ); 	/* we're master */
#ifdef QSPIM_D201_SW
	MWRITE_D8( h->maQspi, QSPI_QILR, 0xf8 );		/* allow qsm interrupts */
#else
	MSETMASK_D8( h->maQspi, QSPI_QILR, 0x38 );		/* allow qsm interrupts */
#endif
	MWRITE_D8( h->maQspi, QSPI_QIVR, 0xff );		/* allow qsm interrupts */
	/* JT 22-01-2015 */
#ifndef GPIO_DEBUG 
	MWRITE_D8( h->maQspi, QSPI_QPAR, 0x7b );		/* define QSM pins */

#else
	MWRITE_D8( h->maQspi, QSPI_QPAR, 0x3b );
#endif	/* GPIO_DEBUG */
	/* JT 22-01-2015 */
#ifndef QSPIM_D201_SW
	MWRITE_D16( h->maQspi, QSPI_TIMER, 0 );			/* stop QSPI timer */
#endif

	GET_PARAM( "WOMQ", 0, val );
	if( (error = QSPIM_SetStat( h, QSPIM_WOMQ, 0, val ))) goto abort;

	GET_PARAM( "BITS", 16, val );
	if( (error = QSPIM_SetStat( h, QSPIM_BITS, 0, val ))) goto abort;

	GET_PARAM( "CPOL", 0, val );
	if( (error = QSPIM_SetStat( h, QSPIM_CPOL, 0, val ))) goto abort;

	GET_PARAM( "CPHA", 0, val );
	if( (error = QSPIM_SetStat( h, QSPIM_CPHA, 0, val ))) goto abort;

	GET_PARAM( "BAUD", 4000000, val );
	if( (error = QSPIM_SetStat( h, QSPIM_BAUD, 0, val ))) goto abort;

	GET_PARAM( "DSCLK", 2000, val );
	if( (error = QSPIM_SetStat( h, QSPIM_DSCLK, 0, val ))) goto abort;

	GET_PARAM( "DTL", 1000, val );
	if( (error = QSPIM_SetStat( h, QSPIM_DTL, 0, val ))) goto abort;

	GET_PARAM( "PCS_DEFSTATE", 0x00, val );
	if( (error = QSPIM_SetStat( h, QSPIM_PCS_DEFSTATE, 0, val ))) goto abort;

	MWRITE_D8( h->maQspi, QSPI_QDDR, 0x7e );		/* pin data direction */

	GET_PARAM( "TIMER_CYCLE_TIME", 500000, val );
	if( (error = QSPIM_SetStat( h, QSPIM_TIMER_CYCLE_TIME, 0, val)))goto abort;

	GET_PARAM( "RCV_FIFO_DEPTH", 10, val );
	if( (error = QSPIM_SetStat( h, QSPIM_RCV_FIFO_DEPTH, 0, val ))) goto abort;


	GET_PARAM( "BLOCKING_READ", 0, val );
	if( (error = QSPIM_SetStat( h, QSPIM_BLOCKING_READ, 0, val ))) goto abort;

	/*--- alloc xmit buffer ---*/
	if( (h->xmtBuf = (u_int8 *)OSS_MemGet( h->osHdl, h->qspiQueueLen*2,
										   &h->xmtAlloc )) == NULL ){
		DBGWRT_ERR((DBH,"*** QSPIM_Init: can't alloc xmitBuf\n" ));
		error = ERR_OSS_MEM_ALLOC;
		goto abort;
	}
	h->xmtBufState = XMT_EMPTY;

#ifdef QSPIM_SUPPORT_A21_DMA
	/*--- alloc recv buffer ---*/
	if( (h->recvBuf = (u_int8 *)OSS_MemGet( h->osHdl, h->qspiQueueLen*2,
											&h->recvAlloc )) == NULL ){
		DBGWRT_ERR((DBH,"*** QSPIM_Init: can't alloc xmitBuf\n" ));
		error = ERR_OSS_MEM_ALLOC;
		goto abort;		
	}

	__DMA_Init( h );			/* prepare for DMA */
#endif
#ifdef QSPIM_SUPPORT_8240_DMA
	__DMA_Init( h );			/* prepare for DMA */
#endif

	*hP = h;	/* set low-level driver handle */
abort:
	if( error )
		Cleanup( h, error );

	DBGWRT_1((DBH,"QSPIM_Init: exit %d\n", error ));
	return(error);
}

/****************************** QSPIM_Exit ***********************************
 *
 *  Description:  De-initialize hardware and clean up memory
 *
 *                The interrupt is disabled.
 *
 *---------------------------------------------------------------------------
 *  Input......:  hP  	pointer to low-level driver handle
 *  Output.....:  return    success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_Exit(
   LL_HANDLE    **hP
)
{
    LL_HANDLE *h = *hP;
	int32 error = 0;

    DBGWRT_1((DBH, "LL - QSPIM_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/
	MCLRMASK_D16( h->maQspi, QSPI_SPCR2, 0x8000 ); 	/* disable QSPI irqs */

	/*--- deactivate SYNCLK/FRAMESYN ---*/
	UPDATE_QPDR( h->qpdrShadow & ~0x0180 );

    /*------------------------------+
    |  clean up memory               |
    +------------------------------*/
	error = Cleanup(h,error);
	*hP = NULL;		/* set low-level driver handle to NULL */

	return(error);
}

/****************************** QSPIM_Read ***********************************
 *
 *  Description:  Not supported for QSPIM driver
 *
 *---------------------------------------------------------------------------
 *  Input......:  h    low-level handle
 *                ch       current channel
 *  Output.....:  valueP   read value
 *                return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_Read(
    LL_HANDLE *h,
    int32 ch,
    int32 *valueP
)
{
	return(ERR_LL_ILL_DIR);
}

/****************************** QSPIM_Write **********************************
 *
 *  Description:  Not supported for QSPIM driver
 *
 *---------------------------------------------------------------------------
 *  Input......:  h    low-level handle
 *                ch       current channel
 *                value    value to write
 *  Output.....:  return   success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_Write(
    LL_HANDLE *h,
    int32 ch,
    int32 value
)
{
	return(ERR_LL_ILL_DIR);
}

/****************************** QSPIM_SetStat ********************************
 *
 *  Description:  Set the driver status
 *
 *  The following status codes are supported:
 *
 *  ACTION CODES:
 *
 *  Code                 Description                     Values
 *  -------------------  ------------------------------  ----------
 *  QSPIM_TIMER_STATE	 start/stop cycle timer			 0/1
 *	QSPIM_FRM_SIG_SET	 install signal for QSPI frame	 signal code
 *	QSPIM_FRM_SIG_CLR	 remove signal for QSPI frame	 don't care
 *	QSPIM_EMG_SIG_SET	 install signal for emergency	 signal code
 *	QSPIM_EMG_SIG_CLR	 remove signal for emergency	 don't care
 *	QSPIM_CLR_RCV_FIFO	 clear receive fifo				 don't care
 *	QSPIM_BLK_CALLBACK	 install/remove callback func	 see below
 *  QPSIM_FRAMESYN		 activate/deactive FRAMESYN pin	 0/1
 *
 *  CONFIGURATION CODES:
 *
 *  Code                 Description                     Values
 *  -------------------  ------------------------------  ----------
 *	QSPIM_WOMQ			 wired OR for QSPI pins			 0/1
 *  QSPIM_BITS			 bits per word					 8..16
 *	QSPIM_CPOL			 clock polarity 				 0/1
 *	QSPIM_CPHA			 clock phase					 0/1
 *	QSPIM_BAUD			 SCLK baudrate					 [Hz]
 *	QSPIM_DSCLK			 PCS to SCLK delay				 [ns]
 *	QSPIM_DTL			 delay after transfer			 [ns]
 *	QSPIM_PCS_DEFSTATE	 default state of PCS3..0		 0x0..0xf
 *	QSPIM_TIMER_LO_TIME	 low time of cycle timer		 [ns]
 *	QSPIM_TIMER_HI_TIME	 high time of cycle timer		 [ns]
 *	QSPIM_RCV_FIFO_DEPTH number of queue entries in fifo 2..n
 *  QSPIM_BLK_DEFINE_FRM defines the frame structure	 see below
 *	QSPIM_NO_DUPLICATION prevent frame duplication		 0/1
 *	QSPIM_AUTOMODE		 use 16Z076 automode      		 0/1
 *  M_LL_DEBUG_LEVEL     driver debug level          	 see dbg.h
 *  M_MK_IRQ_ENABLE      interrupt enable            	 0..1
 *  M_LL_IRQ_COUNT       interrupt counter           	 0..max
 *
 *	QSPIM_TIMER_STATE: A value of 1 starts the cycle timer and therefore
 *		QSPI transmission. A value of 0 stops it.
 *
 *	QSPIM_FRM_SIG_SET: Installs a signal that is sent whenever a QSPI frame
 *		has been transmitted. QSPIM_FRM_SIG_CLR removes this signal.
 *
 *  QSPIM_EMG_SIG_CLR: Installs a signal that is sent whenever a transmit
 *		buffer underrun occurs. QSPIM_EMG_SIG_CLR removes this signal.
 *
 *  QSPIM_CLR_RCV_FIFO: discards all entries in the receive fifo.
 *
 *  QSPIM_WOMQ, QSPIM_BITS, QSPIM_CPOL, QSPIM_CPHA, QSPIM_BAUD, QSPIM_DSCLK,
 *	QSPIM_DTL: same meaning as the corresponding fields in the 68332 QSPI.
 *
 *  QSPIM_PCS_DEFSTATE: defines the state of the PCS3..0 pins when no
 *  transmission is pending.
 *
 *	QSPIM_TIMER_CYCLE_TIME: define cycle time of both high/low phase
 *		(together) of the cycle clock. A value of 500000 will result in
 *		250us low and 250us high time
 *		N.B. No effect in the D201 QSPI implementation!
 *		Timer cycle fixed to 250us low/250 us high time.
 *
 *	QSPIM_RCV_FIFO_DEPTH: reallocates memory for the receive fifo.
 *		Implicitely clears all pending entries
 *
 *	QSPIM_BLK_DEFINE_FRM: Defines the structure of the QSPI frame that
 *		is sent once per cycle. Block setstat. blk->data must be an array
 *		of 16 bit words. Each word is defined as follows:
 *
 *		Bit 15    14    13    12     11    10    9     8       7 .. 0
 *		--------------------------------------------------------------
 *          CONT  BITSE DT    DSCLK  PCS3  PCS2  PCS1  PCS0    NWORDS
 *
 *		CONT:  like in QSPI command ram, only affects last word of slave
 *		BITSE: like in QSPI command ram, affects all words of slave
 *		DT:    like in QSPI command ram, affects all words of slave
 *		DSCLK: like in QSPI command ram, only affects first words of slave
 *		PCS3..0: state of chip select pins during transmission
 *		NWORDS:	 number of QSPI words to transfer to/from slave
 *
 *  QSPIM_BLK_CALLBACK: Can be used to install or remove a callback routine
 *	    within the application that is called whenever a QSPI frame has
 *		been transferred. The <data> element of the M_SG_BLOCK structure
 *		must point to a QSPIM_CALLBACK_PARMS structure.
 *		If the QSPIM_CALLBACK_PARMS.func is zero, the callback is removed.
 *
 *	QSPIM_FRAMESYN: Controls the value of the FRAMESYN signal. The passed
 *		value is output on the next falling edge of the cycle timer.
 *
 *  QSPIM_NO_DUPLICATION: A value of 1 prevents frame duplication when the
 *      xmt buffer is not filled. A value of 0 (the default behaviour)
 *      transmits the last set frame data at each timer IRQ.
 *
 *	QSPIM_AUTOMODE: A value of 1 will set the 16Z076 core into automode. This
 *		means, it will start sending it's internal TXRAM content on a rising
 *		edge of SYNCLOCK, meaning it will inhibit the timer IRQ.
 *
 *---------------------------------------------------------------------------
 *  Input......:  llHdl          ll handle
 *                code           status code
 *                ch             current channel
 *                value32_or_64  data or
 *                               ptr to block data struct (M_SG_BLOCK)  (*)
 *  Output.....:  return     success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_SetStat(
    LL_HANDLE *h,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
	int32		value	= (int32)value32_or_64;	/* 32bit value     */
	INT32_OR_64	valueP	= value32_or_64;		/* stores 32/64bit pointer */
	int32 error = ERR_SUCCESS;
	MACCESS ma = h->maQspi;

    DBGWRT_1((DBH, "LL - QSPIM_SetStat: ch=%d code=0x%04x value=0x%x\n",
			  ch,code,value));

    switch(code) {
	/*----------------+
	|  Action codes   |
	+----------------*/
	case QSPIM_TIMER_STATE:
	{
		OSS_IRQ_STATE irqState;
		/*--- start/stop timer and QSPI ---*/

		DBGWRT_1((DBH, "LL - QSPIM_SetStat: QSPIM_TIMER_STATE\n"));
		if( h->irqEnabled == FALSE ){
			DBGWRT_ERR((DBH,"*** QSPIM_SetStat: irq not enabled!\n"));
			error = ERR_LL_DEV_NOTRDY;
			break;
		}
		irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );		/* mask irqs */
		if( value ){
			if( h->running == FALSE ){
				ClrRcvFifo( h );
				h->running = TRUE;

				if( !h->doAutoMode ){
#ifndef QSPIM_D201_SW
				MWRITE_D16( ma, QSPI_TIMER, 0 ); /* be sure timer stopped */
#endif
				/* clear pending timer IRQ, enable timer IRQ */
				MSETMASK_D8( ma, QSPI_QILR, 0x2 );
				MSETMASK_D8( ma, QSPI_QILR, 0x1 );
				}

				/* activate SYNCLK on next rising timer edge */
				UPDATE_QPDR( h->qpdrShadow | 0x0080 );

				/* enable QSPI interrupts, set NEWQP */
				MCLRMASK_D16( ma, QSPI_SPCR4, 0x00ff );
				MSETMASK_D16( ma, QSPI_SPCR2, 0x8000 );

				/* start timer */
#ifndef QSPIM_D201_SW
				MWRITE_D16( ma, QSPI_TIMER, h->timerValue );
#endif
			}
		}
		else {
			MCLRMASK_D8( ma, QSPI_QILR, 0x1 );  /* disable timer IRQ */
			/*--- deactivate SYNCLK/FRAMESYN ---*/
			UPDATE_QPDR( h->qpdrShadow & ~0x0180 );
			h->running = FALSE;
		}
		OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );	/* unmask irqs */
		break;
	}
	case QSPIM_CLR_RCV_FIFO:
		/*--- clear the receiver fifo ---*/
		ClrRcvFifo( h );

		break;

	case QSPIM_FRM_SIG_SET:
		/*--- install signal for QSPI finished ---*/
		if( h->frmSig ){					/* already installed? */
			error = ERR_OSS_SIG_SET;
			break;
		}

		error = OSS_SigCreate(h->osHdl, value, &h->frmSig );
		break;

	case QSPIM_FRM_SIG_CLR:
		/*--- remove signal for QSPI finished ---*/
		if( h->frmSig == NULL ){		/* really installed? */
			error = ERR_OSS_SIG_CLR;
			break;
		}

		error = OSS_SigRemove(h->osHdl, &h->frmSig );
		break;

	case QSPIM_EMG_SIG_SET:
		/*--- install signal for emergency ---*/
		if( h->emgSig ){					/* already installed? */
			error = ERR_OSS_SIG_SET;
			break;
		}

		error = OSS_SigCreate(h->osHdl, value, &h->emgSig );
		break;

	case QSPIM_EMG_SIG_CLR:
		/*--- remove signal for emergency ---*/
		if( h->emgSig == NULL ){		/* really installed? */
			error = ERR_OSS_SIG_CLR;
			break;
		}

		error = OSS_SigRemove(h->osHdl, &h->emgSig );
		break;

	case QSPIM_FRAMESYN:
	{
		OSS_IRQ_STATE irqState;
		irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );		/* mask irqs */
		/*--- control FRAMESYN signal ---*/
		if( value ){
			UPDATE_QPDR( h->qpdrShadow | 0x100 );
		}
		else {
			UPDATE_QPDR( h->qpdrShadow & ~0x100 );
		}
		OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );	/* unmask irqs */

		break;
	}

	case QSPIM_NO_DUPLICATION:
		h->noFrameDup = TRUE;
		break;

	case M_MK_IRQ_ENABLE:
		if( value ){
			h->irqEnabled = TRUE;
		}
		else {
			/*--------------------------+
			|  disable interrupts       |
			+--------------------------*/
			h->irqEnabled = FALSE;
			MCLRMASK_D8(ma, QSPI_QILR, 0x1 ); 		/* disable timer IRQ */
			MCLRMASK_D8(ma, QSPI_SPSR, 0x80 ); 		/* clear SPIF */
			MCLRMASK_D16(ma, QSPI_SPCR2, 0x8000 ); 	/* clear SPIFIE */
		}
		break;

	case QSPIM_BLK_CALLBACK:
	{
		M_SG_BLOCK *blk = (M_SG_BLOCK *)valueP;
		QSPIM_CALLBACK_PARMS *parms;
		OSS_IRQ_STATE irqState;

		if( blk->size != sizeof(QSPIM_CALLBACK_PARMS)){
			DBGWRT_ERR((DBH,"*** QSPIM_BLK_CALLBACK: bad M_SG_BLOCK size\n"));
			error = ERR_LL_ILL_PARAM;
			break;
		}
		parms = (QSPIM_CALLBACK_PARMS *)blk->data;

		DBGWRT_2((DBH, "QSPIM_BLK_CALLBACK: func=%08p arg=0x%x stat=0x%x\n",
				  parms->func, parms->arg, parms->statics ));

		irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );		/* mask irqs */
		h->callbackFunc 	= parms->func;
		h->callbackArg		= parms->arg;
		h->callbackStatics 	= parms->statics;
		OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );	/* unmask irqs */
		break;
	}
	/*------------------+
	|  QSPI parameters  |
	+------------------*/
	case QSPIM_BLK_DEFINE_FRM:
		NOT_ALLOWED_WHEN_RUNNING;

		error = DefineFrm( h, (M_SG_BLOCK*)valueP);
		break;

	case QSPIM_WOMQ:
		NOT_ALLOWED_WHEN_RUNNING;
        if( value ) {
            MSETMASK_D16( ma, QSPI_SPCR0, 0x4000 );
        }
        else {
			MCLRMASK_D16( ma, QSPI_SPCR0, 0x4000 );
        }
		break;

	case QSPIM_BITS:
		NOT_ALLOWED_WHEN_RUNNING;
		if( value < 8 || value > 16 ){
			DBGWRT_ERR((DBH,"*** QSPIM: Bad number of BITS %d\n", value ));
			error = ERR_LL_ILL_PARAM;
			break;
		}

		MCLRMASK_D16( ma, QSPI_SPCR0, 0x3c00 );
		MSETMASK_D16( ma, QSPI_SPCR0, (value & 0xf)<<10);
		break;

	case QSPIM_CPOL:
		NOT_ALLOWED_WHEN_RUNNING;
        if( value ) {
			MSETMASK_D16( ma, QSPI_SPCR0, 0x200 );
        }
        else {
			MCLRMASK_D16( ma, QSPI_SPCR0, 0x200 );
        }
		break;

	case QSPIM_CPHA:
		NOT_ALLOWED_WHEN_RUNNING;
        if( value ) {
			MSETMASK_D16( ma, QSPI_SPCR0, 0x100 );
        }
        else {
			MCLRMASK_D16( ma, QSPI_SPCR0, 0x100 );
        }
		break;

	case QSPIM_BAUD:
		NOT_ALLOWED_WHEN_RUNNING;
		{
			u_int32 baudCode;

			if( value == 0 || (value > h->pldClock/2)){
				DBGWRT_ERR((DBH,"*** QSPIM: illegal baud %d\n", value));
				error = ERR_LL_ILL_PARAM;
				break;
			}

			baudCode = h->pldClock/value;

			if( baudCode&0x1 )
				baudCode = (baudCode>>1)+1;
			else
				baudCode = (baudCode>>1);

			if( baudCode > 0xff ){
				DBGWRT_ERR((DBH,"*** QSPIM: illegal baud %d\n", value));
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH," baudCode=%d\n", baudCode ));

			MCLRMASK_D16( ma, QSPI_SPCR0, 0x00ff );
			MSETMASK_D16( ma, QSPI_SPCR0, baudCode);

		}
		break;

	case QSPIM_DSCLK:
		NOT_ALLOWED_WHEN_RUNNING;
		{
			u_int32 dsclk;

			if(  value == 0 ){
				DBGWRT_ERR((DBH,"*** QSPIM: illegal dsclk %d\n", value));
				error = ERR_LL_ILL_PARAM;
				break;
			}

			dsclk = h->pldClock / 10000 * value / 100000;

			if( (value < 1000000000/h->pldClock) || (dsclk > 128)){
				DBGWRT_ERR((DBH,"*** QSPIM: illegal dsclk %d\n", value));
				error = ERR_LL_ILL_PARAM;
				break;
			}

			if( dsclk == 0 ) dsclk = 1;	/* insert minimum delay */

			DBGWRT_2((DBH," dsclk=%d\n", dsclk ));

			MCLRMASK_D16( ma, QSPI_SPCR1, 0x7f00 );
			MSETMASK_D16( ma, QSPI_SPCR1, (dsclk & 0x7f)<<8);
		}
		break;

	case QSPIM_DTL:
		NOT_ALLOWED_WHEN_RUNNING;
		{
			u_int32 dtl;

			if( value == 0 ){
				DBGWRT_ERR((DBH,"*** QSPIM: illegal dtl %d\n", value));
				error = ERR_LL_ILL_PARAM;
				break;
			}

			dtl = h->pldClock / 10000 * value / 3200000;

			if( (value < 1000000000/h->pldClock) || (dtl > 256)){
				DBGWRT_ERR((DBH,"*** QSPIM: illegal dtl %d\n", value));
				error = ERR_LL_ILL_PARAM;
				break;
			}

			if( dtl == 0 ) 	dtl = 1;	/* insert minimum delay */

			DBGWRT_2((DBH," dtl=%d\n", dtl ));

			MCLRMASK_D16( ma, QSPI_SPCR1, 0x00ff );
			MSETMASK_D16( ma, QSPI_SPCR1, dtl & 0xff);
		}
		break;

	case QSPIM_PCS_DEFSTATE:
		NOT_ALLOWED_WHEN_RUNNING;
		{
			u_int16 qpdr;
			OSS_IRQ_STATE irqState;

			qpdr = MREAD_D16( ma, QSPI_QPDR );
			h->pcsDefState = (u_int8)(value & 0xf);
			h->qpdrShadow &= ~0x78;
			irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );		/* mask irqs */
			UPDATE_QPDR( h->qpdrShadow | (h->pcsDefState << 3) );
			OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );  /* unmask irqs */
		}
		break;

	case QSPIM_TIMER_CYCLE_TIME:
		NOT_ALLOWED_WHEN_RUNNING;
#ifndef QSPIM_D201_SW
		{
			u_int32 reg = value / 1000 / 50;

			if( reg & 0x1 )
				reg += 2;
			reg >>= 1;
			reg--;

			if( (reg > 0xf) || (reg == 0) ){
				DBGWRT_ERR((DBH,"*** QSPIM: illegal timer time %d\n", value));
				error = ERR_LL_ILL_PARAM;
				break;
			}

			DBGWRT_2((DBH," timer cycle = %d\n", reg ));
			h->timerValue = reg;

		}
#endif
		break;

	case QSPIM_RCV_FIFO_DEPTH:
		error = CreateRcvFifo( h, value );
		break;

	case QSPIM_BLOCKING_READ:
		h->doBlockRead = value;
		break;

	case QSPIM_AUTOMODE:
		if (value == 1) {
			OSS_IRQ_STATE irqState;
			u_int8 qilr;

			irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );		/* mask irqs */
			qilr = MREAD_D8( h->maQspi, QSPI_QILR );
			qilr |= 0x80;		/* Set Automode */
			qilr &= ~0x01; 		/* Disable the QSPI Timer */
			MWRITE_D8( h->maQspi, QSPI_QILR, qilr  );

			/* Activate SYNCLK */
			UPDATE_QPDR( h->qpdrShadow | 0x0080 );
			MSETMASK_D16(ma, QSPI_SPCR2, 0x8000 ); 	/* set SPIFIE */

			OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );	/* unmask irqs */
		}
		else {
			OSS_IRQ_STATE irqState;
			u_int8 qilr;

			irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );		/* mask irqs */
			qilr = MREAD_D8( h->maQspi, QSPI_QILR );
			qilr &= ~0x80;
			MWRITE_D8( h->maQspi, QSPI_QILR, qilr  );
			OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );	/* unmask irqs */
		}

		h->doAutoMode = value;			
		break;
	case M_LL_DEBUG_LEVEL: h->dbgLevel = value; break;
	case M_LL_IRQ_COUNT:   h->irqCount = value; break;

	default:
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
		error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/****************************** QSPIM_GetStat ********************************
 *
 *  Description:  Get the driver status
 *
 *  The following status codes are supported:
 *
 *  Code                 Description                     Values
 *  -------------------  ------------------------------  ----------
 *	QSPIM_TIMER_STATE	 get state of timer QSPI		 0/1
 *  QSPIM_RCV_FIFO_COUNT number of frames in rcv fifo
 *  QSPIM_ERRORS		 get/reset errors				 QSPIM_ERR_xxx
 *	QSPIM_WOMQ			 wired OR for QSPI pins			 0/1
 *  QSPIM_BITS			 bits per word					 8..16
 *	QSPIM_CPOL			 clock polarity 				 0/1
 *	QSPIM_CPHA			 clock phase					 0/1
 *	QSPIM_BAUD			 SCLK baudrate					 [Hz]
 *	QSPIM_DSCLK			 PCS to SCLK delay				 [ns]
 *	QSPIM_DTL			 delay after transfer			 [ns]
 *	QSPIM_PCS_DEFSTATE	 default state of PCS3..0		 0x0..0xf
 *	QSPIM_TIMER_CYCLE_TIME cycle time					 [ns]
 *	QSPIM_RCV_FIFO_DEPTH number of queue entries in fifo 2..n
 *	QSPIM_BLK_DIRECT_WRITE_FUNC determine write func ptr see below
 *	QSPIM_BLK_DIRECT_ISETSTAT_FUNC determine setstat func ptr see below
 *
 *  M_LL_DEBUG_LEVEL     driver debug level          	 see dbg.h
 *  M_LL_CH_NUMBER       number of channels          	 1
 *  M_LL_CH_DIR          direction of curr. chan.    	 M_CH_BINARY
 *  M_LL_CH_LEN          length of curr. ch. [bits]  	 16
 *  M_LL_CH_TYP          description of curr. chan.  	 M_CH_INOUT
 *  M_LL_IRQ_COUNT       interrupt counter           	 0..max
 *  M_MK_BLK_REV_ID      ident function table ptr    	 -
 *
 *  QSPIM_TIMER_STATE: gets the current state of cycle timer/QPSI. 0 means
 *		stopped, 1 means running.
 *
 *  QSPIM_RCV_FIFO_COUNT: get the number of frames in rcv fifo
 *
 *	QSPIM_ERRORS: reads the error flags and clears the internal error flags.
 *	   	return value is a ORed combination of QSPIM_ERR_xxx flags.
 *
 *  QSPIM_BLK_DIRECT_WRITE_FUNC: Used to get entry point and argument for
 *		the direct write func. The <data> element of the M_SG_BLOCK structure
 *		must point to a QSPIM_DIRECT_WRITE_PARMS structure that is filled
 *		by that call.
 *
 *  QSPIM_BLK_DIRECT_ISETSTAT_FUNC: Used to get entry point and argument for
 *		the setstat function that can be called by user interrupt service
 *		routines. The <data> element of the M_SG_BLOCK structure
 *		must point to a QSPIM_DIRECT_ISETSTAT_PARMS structure that is filled
 *		by that call.
 *
 *	Rest of getstats correspond to the setstats with the same name
 *---------------------------------------------------------------------------
 *  Input......:  llHdl           ll handle
 *                code            status code
 *                ch              current channel
 *                value32_or_64P  ptr to block data struct (M_SG_BLOCK)  (*)
 *                (*) = for block status codes
 *  Output.....:  value32_or_64P  data ptr or
 *                                ptr to block data struct (M_SG_BLOCK)  (*)
 *                return          success (0) or error code
 *                (*) = for block status codes
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_GetStat(
    LL_HANDLE *h,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
	int32 *valueP		  = (int32*)value32_or_64P;	/* pointer to 32bit value  */
	INT32_OR_64	*value64P = value32_or_64P;		 	/* stores 32/64bit pointer  */
	/* M_SG_BLOCK	*blk	  = (M_SG_BLOCK*)value32_or_64P;  stores block struct pointer */
	MACCESS ma = h->maQspi;

	int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - QSPIM_GetStat: ch=%d code=0x%04x\n",
			  ch,code));

    switch(code)
    {
	/*----------------------+
	|	Query parameters    |
	+----------------------*/
	case QSPIM_TIMER_STATE:
		*valueP = h->running;
		break;

	case QSPIM_RCV_FIFO_COUNT:
		*valueP = h->rcvFifoCount;
		break;

	case QSPIM_ERRORS:
		*valueP = h->errors;
		h->errors = 0;
		break;

	case QSPIM_BLK_DIRECT_WRITE_FUNC:
	{
		M_SG_BLOCK *blk = (M_SG_BLOCK *)value64P;
		QSPIM_DIRECT_WRITE_PARMS *parms;

		if( blk->size != sizeof(QSPIM_DIRECT_WRITE_PARMS)){
			DBGWRT_ERR((DBH,"*** QSPIM_BLK_DIRECT_WRITE_FUNC: "
						"bad M_SG_BLOCK size\n"));
			error = ERR_LL_ILL_PARAM;
			break;
		}
		parms = (QSPIM_DIRECT_WRITE_PARMS *)blk->data;

		parms->func = DirectWriteFunc;
		parms->arg	= (void *)h;
		break;
	}

	case QSPIM_BLK_DIRECT_ISETSTAT_FUNC:
	{
		M_SG_BLOCK *blk = (M_SG_BLOCK *)value64P;
		QSPIM_DIRECT_ISETSTAT_PARMS *parms;

		if( blk->size != sizeof(QSPIM_DIRECT_ISETSTAT_PARMS)){
			DBGWRT_ERR((DBH,"*** QSPIM_BLK_DIRECT_ISETSTAT_FUNC: "
						"bad M_SG_BLOCK size\n"));
			error = ERR_LL_ILL_PARAM;
			break;
		}
		parms = (QSPIM_DIRECT_ISETSTAT_PARMS *)blk->data;

		parms->func = DirectISetstat;
		parms->arg	= (void *)h;
		break;
	}

	/*----------------------+
	|  QSPI parameters      |
	+----------------------*/
	case QSPIM_WOMQ:
		*valueP = !!(MREAD_D16( ma, QSPI_SPCR0 ) & 0x4000);
		break;

	case QSPIM_BITS:
		*valueP = (MREAD_D16( ma, QSPI_SPCR0 ) & 0x3c00) >> 10;
		if( *valueP == 0 )
			*valueP = 16;
		break;

	case QSPIM_CPOL:
		*valueP = !!(MREAD_D16( ma, QSPI_SPCR0 ) & 0x200);
		break;

	case QSPIM_CPHA:
		*valueP = !!(MREAD_D16( ma, QSPI_SPCR0 ) & 0x100);
		break;

	case QSPIM_BAUD:
		*valueP = h->pldClock / (MREAD_D16( ma, QSPI_SPCR0 ) & 0xff) / 2;
		break;

	case QSPIM_DSCLK:
	{
		u_int16 dsclk = (MREAD_D16( ma, QSPI_SPCR1 )&0x7f00)>>8;
		if( dsclk == 0 ) dsclk = 128;

		*valueP = 100000 * dsclk / (h->pldClock/10000);
		break;
	}
	case QSPIM_DTL:
	{
		u_int16 dtl = (MREAD_D16( ma, QSPI_SPCR1 )&0xff);
		if( dtl == 0 ) dtl = 256;
		*valueP = 3200000 * dtl / (h->pldClock/10000);
		break;
	}
	case QSPIM_PCS_DEFSTATE:
		*valueP = h->pcsDefState;
		break;

	case QSPIM_TIMER_CYCLE_TIME:
#ifndef QSPIM_D201_SW
		*valueP = (h->timerValue + 1) * 100000;
#else
		*valueP = 500000;		/* time fixed on D201 */
#endif
		break;

	case QSPIM_RCV_FIFO_DEPTH:
		*valueP = h->rcvFifoDepth;
		break;

	case QSPIM_BLOCKING_READ:
		*valueP = h->doBlockRead;
		break;

	case QSPIM_AUTOMODE:
		*valueP = h->doAutoMode;
		break;
    /*--------------------------+
	|  Misc                     |
	+--------------------------*/
	case M_LL_DEBUG_LEVEL:	*valueP = h->dbgLevel;				        break;
	case M_LL_CH_NUMBER:    *valueP = CH_NUMBER;					    break;
	case M_LL_CH_DIR:		*valueP = M_CH_INOUT;					    break;
	case M_LL_CH_LEN:       *valueP = 16;							    break;
	case M_LL_CH_TYP:		*valueP = M_CH_BINARY;					    break;
	case M_LL_IRQ_COUNT:	*valueP = h->irqCount;				        break;
	case M_MK_BLK_REV_ID:	*value64P = (INT32_OR_64)&h->idFuncTbl;		break;

	default:
		/*--------------------------+
        |  (unknown)                |
        +--------------------------*/
		error = ERR_LL_UNK_CODE; break;
    }

	return(error);
}

/******************************* QSPIM_BlockRead *****************************
 *
 *  Description:  Read QSPI frame from driver receive fifo
 *
 *  Always non blocking. If no frame present in receive fifo, returns without
 *	error and number of read bytes is zero.
 *
 *  <buf> will have the same format as the QSPI receive RAM
 *
 *  Returns ERR_LL_READ if the fifo has overflowed. In this case, application
 *  must call Getstat QSPIM_ERRORS to clear the error.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h        	   low-level handle
 *                ch           current channel
 *                buf          data buffer
 *                size         data buffer size
 *  Output.....:  nbrRdBytesP  number of read bytes
 *                return       success (0) or error code
 *							   ERR_LL_READ: fifo overrun occurred
 *							   ERR_LL_USERBUF: user buffer too small for frm
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_BlockRead(
     LL_HANDLE *h,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
    DBGWRT_1((DBH, "LL - QSPIM_BlockRead: ch=%d, size=%d\n",ch,size));

	if ( h->doBlockRead ) {
		int32 error;

		error = OSS_SemWait(h->osHdl, h->readSemHdl, OSS_SEM_WAITFOREVER);
		if( error ) {
			DBGWRT_ERR((DBH,
						"*** QSPIM_BlockRead: Error waiting for Semaphore %d\n",
						error));
			return ERR_LL_READ;
		}
#ifdef QSPIM_SUPPORT_A21_DMA
		/* If we're using the blocking read mode in combination with the batch
		 * DMA, we do not take the detour and use the receive FIFOs, but try to
		 * get the data out of the driver as fast as possible without
		 * introducing additional receive latencys. That's not quite zero-copy
		 * but we're getting near to that.
		 */
		CopyBlock( (u_int32 *)h->recvBuf, (u_int32 *)buf, h->frmLen );
		*nbrRdBytesP = h->frmLen;
		return ERR_SUCCESS;
#endif
		
	}

	if( h->errors & QSPIM_ERR_RCV_FIFO_OVER ){
		*nbrRdBytesP = 0;
		DBGWRT_ERR((DBH,"*** QSPIM_BlockRead: FIFO overrun pending\n"));

		return ERR_LL_READ;
	}

	if( h->rcvFifoCount == 0 ){
		/*--- FIFO empty ---*/
		DBGWRT_3((DBH," no entries in fifo\n"));
		*nbrRdBytesP = 0;
	}
	else {
		OSS_IRQ_STATE irqState;
		/*--- copy one frame from driver fifo to user buffer ---*/
		if( (u_int32)size < h->frmLen ){
			DBGWRT_ERR((DBH,"*** QSPIM_BlockRead: user buf too small %d\n",
						size ));
			return ERR_LL_USERBUF;
		}
		CopyBlock( (u_int32 *)h->rcvFifoNxtOut, (u_int32 *)buf, h->frmLen );

		/*--- advance next out pointer ---*/
		h->rcvFifoNxtOut += h->qspiQueueLen*2;
		if( h->rcvFifoNxtOut >= h->rcvFifoEnd )
			h->rcvFifoNxtOut = h->rcvFifoStart;

		/*--- update fifo entry count ---*/
		irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );
		h->rcvFifoCount--;
		OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );

		*nbrRdBytesP = h->frmLen;
	}

	return(ERR_SUCCESS);
}

/********************************* DirectWriteFunc ***************************
 *
 *  Description: Entry point for application when bypassing MDIS
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: arg			the LL handle
 *				 qspiFrame		ptr to data to be sent
 *				 qspiFrameLen	number of bytes in qspiFrame
 *  Output.....: returns		error code
 *  Globals....: -
 ****************************************************************************/
static int32 DirectWriteFunc(
	void *arg,
	u_int16 *qspiFrame,
	u_int32 qspiFrameLen)
{
	int32 dummy;

	return QSPIM_BlockWrite( (LL_HANDLE *)arg, 0,
							 (void *)qspiFrame, (int32)qspiFrameLen,
							 &dummy );
}

/****************************** DirectISetstat ********************************
 *
 *  Description:  Set the driver status from an user interrupt routine
 *
 *  The following status codes are supported:
 *
 *  ACTION CODES:
 *
 *  Code                 Description                     Values
 *  -------------------  ------------------------------  ----------
 *  QPSIM_FRAMESYN		 activate/deactive FRAMESYN pin	 0/1
 *
 *	QSPIM_FRAMESYN: Controls the value of the FRAMESYN signal. The passed
 *		value is output on the next falling edge of the cycle timer.
 *---------------------------------------------------------------------------
 *  Input......: arg			the LL handle
 *				 qspiFrame		ptr to data to be sent
 *				 qspiFrameLen	number of bytes in qspiFrame
 *  Output.....: returns		error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 DirectISetstat(
	void   *arg,
    int32  code,
    int32  ch,
    int32  value
)
{
	LL_HANDLE *h = (LL_HANDLE *)arg;
	int32 error = ERR_SUCCESS;

    DBGWRT_1((DBH, "LL - ISetStat: ch=%d code=0x%04x value=0x%x\n",
			  ch,code,value));

    switch(code) {
	/*----------------+
	|  Action codes   |
	+----------------*/
	case QSPIM_FRAMESYN:
		/*--- control FRAMESYN signal ---*/
		if( value ){
			UPDATE_QPDR( h->qpdrShadow | 0x100 );
		}
		else {
			UPDATE_QPDR( h->qpdrShadow & ~0x100 );
		}
		break;

	default:
        /*--------------------------+
        |  (unknown)                |
        +--------------------------*/
		error = ERR_LL_UNK_CODE;
    }

	return(error);
}

#define OUT(hw,mem) (void)((*(volatile u_int32 *) (hw)) = (mem))

#define MBLOCK_WRITE_BE32(ma,offs,size,src)	\
  { int sz=size>>2;							\
    u_int32 *mem=(u_int32 *)src;			\
    unsigned long hw = (MACCESS)(ma)+(offs)+_MAC_OFF_;	\
    while(sz--){							\
      OUT(hw,QSPISWAP(*mem));				\
      mem++;								\
      hw += 4;								\
    }										\
  }

/****************************** QSPIM_DirectCopy *****************************
 *
 *  Description:  Write QSPI frame into FPGA internal buffer
 *
 *  Always non blocking.
 *
 *  The buffered frame will then be transferred to the QSPI slaves on the next
 *  cycle. It also copies the frame into the xmtBuf and sets it's state to
 *  XMT_FILLED. This is needed in automode to be able to 
 *
 *  <buf> must have the same format as the QSPI transmit RAM.
 *  <size> must have exactly the number of bytes of the QSPI frame
 *---------------------------------------------------------------------------
 *  Input......:  h        	   low-level handle
 *                buf          data buffer
 *                size         data buffer size (bytes)
 *  Output.....:  nbrWrBytesP  number of written bytes
 *                return       success (0) or error code
 *							   ERR_LL_WRITE: Write occured during transmission
 *							   ERR_LL_USERBUF: buffer size didn't match
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_DirectCopy(
     LL_HANDLE *h,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
	u_int16 spcr1;

    DBGWRT_1((DBH, "LL - QSPIM_DirectCopy: size=%d\n",size));

	spcr1 = MREAD_D16( h->maQspi, QSPI_SPCR1);
	if ( spcr1 & 0x8000 ) {
		DBGWRT_ERR((DBH, "LL - QSPIM_DirectCopy: QSPI already running"));
		return ERR_LL_WRITE;
	}

	if( size != h->frmLen ){
		DBGWRT_ERR((DBH,"*** QSPIM_DirectCopy: wrong size %d, frmLen=%d\n",
					size, h->frmLen ));
		return ERR_LL_USERBUF;
	}

	/* return number of written bytes */
	*nbrWrBytesP = size;

	IDBGDMP_3((DBH,"Send Data",(void*)buf,h->frmLen,2));
	/*--- copy the application buffer to the QSPI transmit RAM ---*/
	MBLOCK_WRITE_BE32( h->maQspi, QSPI_ETRANRAM,
					  (h->frmLen&0x3 ? h->frmLen&=~0x3, h->frmLen+4 : h->frmLen),
					  buf );

	spcr1 = MREAD_D16( h->maQspi, QSPI_SPCR1);
	if ( spcr1 & 0x8000 ) {
		DBGWRT_ERR((DBH, "LL - QSPIM_DirectCopy: QSPI running before setting NEWQP"));
		return ERR_LL_WRITE;
	}

	/* set NEWQP to 0 */
	MWRITE_D8(h->maQspi, QSPI_SPCR4, 0x00 );

	/* Do double buffering magic */
	h->xmtBufState = XMT_DOUBLE;

	return ERR_SUCCESS;
}
/****************************** QSPIM_BlockWrite *****************************
 *
 *  Description:  Write QSPI frame into driver internal buffer
 *
 *  Always non blocking.
 *
 *  The buffered frame will then be transferred to the QSPI slaves in the
 *  next cycle.
 *
 *  <buf> must have the same format as the QSPI transmit RAM.
 *  <size> must have exactly the number of bytes of the QSPI frame
 *---------------------------------------------------------------------------
 *  Input......:  h        	   low-level handle
 *                ch           current channel (ignored)
 *                buf          data buffer
 *                size         data buffer size (bytes)
 *  Output.....:  nbrWrBytesP  number of written bytes
 *                return       success (0) or error code
 *							   ERR_LL_WRITE:
 *							   ERR_LL_USERBUF: buffer size didn't match
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_BlockWrite(
     LL_HANDLE *h,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
    DBGWRT_1((DBH, "LL - QSPIM_BlockWrite: ch=%d, size=%d\n",ch,size));
	/*
	 * if automode is active, the timer is not started yet and the xmtBuf is
	 * empty, then copy directly into ETRANRAM and not into internal buffer.
	 */
	if( !h->running && h->doAutoMode && h->xmtBufState == XMT_EMPTY ){
		return QSPIM_DirectCopy(h, buf, size, nbrWrBytesP);
	}

	/* If we're in double buffering state then set the xmtBufState to empty and
	 * write to xmtBuf
	 */
	if( h->xmtBufState == XMT_DOUBLE ) {
		DBGWRT_1((DBH, "LL - QSPIM_BlockWrite: 2nd BlockWrite and double buffering\n")); 
		h->xmtBufState = XMT_EMPTY;
	}

	/*
	 * if buffer has already been filled in this cycle return error
	 */
	if( h->xmtBufState != XMT_EMPTY ){
		DBGWRT_ERR((DBH,"*** QSPIM_BlockWrite: buffer already filled in"
					"this cycle\n"));
		return ERR_LL_WRITE;
	}

	if( size != h->frmLen ){
		DBGWRT_ERR((DBH,"*** QSPIM_BlockWrite: wrong size %d, frmLen=%d\n",
					size, h->frmLen ));
		return ERR_LL_USERBUF;
	}

	/* return number of written bytes */
	*nbrWrBytesP = size;

	/*--- flag that xmitbuffer is being filled ---*/
	h->xmtBufState = XMT_FILLING;

	/*--- copy data to driver buffer ---*/
	CopyBlock( (u_int32 *)buf, (u_int32 *)h->xmtBuf, size );

	/*
	 * Check if new cycle has interrupted copying. Return error if so
	 */
	if( h->xmtBufState != XMT_FILLING ){
		DBGWRT_ERR((DBH,"*** QSPIM_BlockWrite: new cycle during copy"));
		return ERR_LL_WRITE;
	}


	h->xmtBufState = XMT_FILLED;

	return(ERR_SUCCESS);
}


/****************************** QSPIM_Irq *************************************
 *
 *  Description:  Interrupt service routine
 *
 *                The interrupt is triggered when a timer interrupt or
 *				  a QSPI interrupt occurs
 *
 *                If the driver can detect the interrupt's cause it returns
 *                LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *
 *---------------------------------------------------------------------------
 *  Input......:  h    	   low-level handle
 *  Output.....:  return   LL_IRQ_DEVICE	irq caused by device
 *                         LL_IRQ_DEV_NOT   irq not caused by device
 *                         LL_IRQ_UNKNOWN   unknown
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_Irq(
   LL_HANDLE *h
)
{
	MACCESS ma = h->maQspi;
	u_int8	doTransfer = TRUE;
	u_int8 qspi_spsr;

    if( MREAD_D8( ma, QSPI_QILR ) & 0x2 ) {	/* timer IRQ pending? */
		/*------------+
		|  Timer IRQ  |
		+------------*/
		IDBGWRT_1((DBH, ">>> QSPIM_Irq: Timer\n"));

		h->irqCount++;
		MCLRMASK_D8( ma, QSPI_QILR, 0x1 );  /* disable timer IRQ */
		MSETMASK_D8( ma, QSPI_QILR, 0x2 ); /* service timer IRQ */

		/*--- copy the driver buffer to the QSPI transmit RAM ---*/
		if( h->xmtBufState == XMT_FILLED ){
#ifdef QSPIM_SUPPORT_8240_DMA
			__DMA_Transfer( h, h->xmtBuf, (char *)ma + QSPI_ETRANRAM,
							(h->frmLen&0x3 ? h->frmLen&=~0x3, h->frmLen+4 : h->frmLen), 1 );
#else
#ifdef QSPIM_Z076
			MBLOCK_WRITE_D16( ma, QSPI_ETRANRAM,
							  h->frmLen&0x1 ? h->frmLen+1 : h->frmLen,
							  (u_int16*)h->xmtBuf );
#else
            MBLOCK_WRITE_D32( ma, QSPI_ETRANRAM,
                              (h->frmLen&0x3 ? h->frmLen&=~0x3, h->frmLen+4 : h->frmLen),
                              h->xmtBuf );
#endif /* QSPIM_Z076 */
#endif /* QSPIM_SUPPORT_8240_DMA */
			IDBGDMP_3((DBH,"Send Data",(void*)h->xmtBuf,h->frmLen,2));
		}
		else {
			IDBGWRT_ERR((DBH,"*** QSPIM_Irq: xmt buf not filled\n"));

			/* send emergency signal to user app */

			if( h->emgSig )
				OSS_SigSend( h->osHdl, h->emgSig );

			/* prevent frame duplication? */
			if( h->noFrameDup )
				doTransfer = FALSE;
		}
		h->xmtBufState = XMT_EMPTY; /* buffer no longer filled */

		if( doTransfer ){
			/*--- start QSPI transfer ---*/
#ifdef QSPIM_Z076
			MWRITE_D8(ma, QSPI_SPCR4, 0x00 );		/* clr NEWQP */
#else
			MWRITE_D8(ma, QSPI_SPCR4 + 1, 0x00 );	/* clr NEWQP */
#endif /* QSPIM_Z076 */
			MSETMASK_D16( ma, QSPI_SPCR1, 0x8000 );	/* set SPE */
		}

		MSETMASK_D8( ma, QSPI_QILR, 0x1 );  /* disable timer IRQ */
		return LL_IRQ_DEVICE;


	} else if( (qspi_spsr = MREAD_D8( ma, QSPI_SPSR)) & 0x80 ) { /* SPIF set? */
		u_int8 *copyBuf;
		u_int8 rtv;

#ifdef GPIO_DEBUG
		/* JT 22-01-2015 */
		UPDATE_QPDR(h->qpdrShadow | (1 << 6) );
		/* JT 22-01-2015 */
#endif	/* GPIO_DEBUG */
		/*------------+
		|  QSPI IRQ   |
		+------------*/
		IDBGWRT_1((DBH, ">>> QSPIM_Irq: QSPI finished\n"));

		h->irqCount++;

		MWRITE_D8( ma, QSPI_SPSR, 0x00 ); /* clear SPIF, service IRQ */

		/*--- check for receive fifo overrun ---*/
		if( h->rcvFifoCount >= h->rcvFifoDepth ){
			IDBGWRT_ERR((DBH,"*** QSPIM_Irq: receive FIFO overrun!\n"));
			h->errors |= QSPIM_ERR_RCV_FIFO_OVER;
			copyBuf = h->rcvOverBuf; /* copy to overrun buffer */
		}
		else {
			copyBuf = h->rcvFifoNxtIn;
			/*--- advance next in pointer ---*/
			h->rcvFifoNxtIn += h->qspiQueueLen*2;
			if( h->rcvFifoNxtIn >= h->rcvFifoEnd )
				h->rcvFifoNxtIn = h->rcvFifoStart;

			h->rcvFifoCount++;	/* update number of entries */


		}
		/*--- copy QSPI receive RAM to receive fifo or overrun buffer---*/
#if defined(QSPIM_SUPPORT_8240_DMA)  
	    __DMA_Transfer( h, (char *)ma + QSPI_ERECRAM, copyBuf,
						(h->frmLen&0x3 ? h->frmLen&=~0x3, h->frmLen+4 : h->frmLen), 2 );
#elif defined (QSPIM_SUPPORT_A21_DMA)
		if( h->xmtBufState == XMT_FILLED ){
			__DMA_Transfer( h );
			MWRITE_D8(ma, QSPI_SPCR4, 0x00 );		/* clear NEWQP */
			h->xmtBufState = XMT_EMPTY;
			IDBGDMP_1((DBH, "Recv Data", (void*)h->recvBuf, h->frmLen, 2));
			/* We don't use the rcvFifo so reset the size */
			h->rcvFifoCount = 0;
		}
		else {
			if( h->emgSig ){
				OSS_SigSend( h->osHdl, h->emgSig );
			}
		}
#else  /* QSPIM_SUPPORT_8240_DMA || QSPIM_SUPPORT_A21_DMA */
#ifdef QSPIM_Z076
		MBLOCK_READ_D16( ma, QSPI_ERECRAM,
							 h->frmLen&0x1 ? h->frmLen+1 : h->frmLen,
							 (u_int16 *)copyBuf );
#else
        MBLOCK_READ_D32( ma, QSPI_ERECRAM,
                             (h->frmLen & 0x3 ? h->frmLen&=~0x3, h->frmLen+4 : h->frmLen),
                             copyBuf );
#endif /* QSPIM_Z076 */
#endif /* QSPIM_SUPPORT_8240_DMA */

		IDBGDMP_3((DBH,"Recv Data",(void*)copyBuf,h->frmLen,2));
		/*--- call the application's callback routine if installed ---*/
		if( h->callbackFunc ){
#if defined(OS9000)
			void *oldStat = change_static( h->callbackStatics );
#endif
			IDBGWRT_2((DBH," call callback func=%08p arg=0x%x stat=0x%x\n",
					   h->callbackFunc, h->callbackArg, h->callbackStatics ));

			h->callbackFunc( h->callbackArg, (u_int16 *)copyBuf, h->frmLen );
#if defined(OS9000)
			change_static( oldStat );
#endif
		}

		/*--- Send signal to user process if installed ---*/
		if( h->frmSig ){
			OSS_SigSend( h->osHdl, h->frmSig );
		}
		/* Check for Realtime Violation Bit */
		rtv = !!(qspi_spsr & QSPIM_RTV);

		/*--- Wake up read function if needed ---*/
		if ( h->doBlockRead ) {
			IDBGWRT_3((DBH, " >>> Wakeup read function\n"));
			OSS_SemSignal(h->osHdl, h->readSemHdl);
		}
		/*--- Check if Realtime violation bit is set by core ---*/
		if( h->emgSig && rtv )
			OSS_SigSend( h->osHdl, h->emgSig );

#ifdef GPIO_DEBUG
		/* JT 22-01-2015 */
		UPDATE_QPDR(h->qpdrShadow & ~(1 << 6) );
		/* JT 22-01-2015 */
#endif	/* GPIO_DEBUG */

		return LL_IRQ_DEVICE;
	}
	else
		return(LL_IRQ_DEV_NOT);		/* not my interrupt */
}

/****************************** QSPIM_Info ************************************
 *
 *  Description:  Get information about hardware and driver requirements
 *
 *                The following info codes are supported:
 *
 *                Code                      Description
 *                ------------------------  -----------------------------
 *                LL_INFO_HW_CHARACTER      hardware characteristics
 *                LL_INFO_ADDRSPACE_COUNT   nr of required address spaces
 *                LL_INFO_ADDRSPACE         address space information
 *                LL_INFO_IRQ               interrupt required
 *                LL_INFO_LOCKMODE          process lock mode required
 *
 *                The LL_INFO_HW_CHARACTER code returns all address and
 *                data modes (ORed) which are supported by the hardware
 *                (MDIS_MAxx, MDIS_MDxx).
 *
 *                The LL_INFO_ADDRSPACE_COUNT code returns the number
 *                of address spaces used by the driver.
 *
 *                The LL_INFO_ADDRSPACE code returns information about one
 *                specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *                data mode represents the widest hardware access used by
 *                the driver.
 *
 *                The LL_INFO_IRQ code returns whether the driver supports an
 *                interrupt routine (TRUE or FALSE).
 *
 *                The LL_INFO_LOCKMODE code returns which process locking
 *                mode the driver needs (LL_LOCK_xxx).
 *
 *---------------------------------------------------------------------------
 *  Input......:  infoType	   info code
 *                ...          argument(s)
 *  Output.....:  return       success (0) or error code
 *  Globals....:  ---
 ****************************************************************************/
static int32 QSPIM_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
		/*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)    |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);

			*addrModeP = MDIS_MA24;
			*dataModeP = MDIS_MD08 | MDIS_MD16 | MDIS_MD32;
			break;
	    }
		/*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
	    }
		/*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);

			switch( addrSpaceIndex ){
			case 0:
#ifdef QSPIM_D201_SW
				/*--- PLX runtime regs ---*/
				*addrModeP = MDIS_MA24;
				*dataModeP = MDIS_MD08 | MDIS_MD16 | MDIS_MD32;
				*addrSizeP = 0x80;
				break;

			case 1:
#endif
				/*--- 16Z076 QSPIM ---*/
				*addrModeP = MDIS_MA_CHAMELEON;
				*dataModeP = MDIS_MD_CHAM_0;
				*addrSizeP = 0x800;
				break;
#ifdef QSPIM_SUPPORT_A21_DMA
			case 1:	/* 16Z062 DMA */
				*addrModeP = MDIS_MA_CHAMELEON;
				*dataModeP = MDIS_MD_CHAM_1;
				*addrSizeP = ADDR_SIZE_SPC_1;
				break;
			case 2:	/* 16Z024_SRAM */
				*addrModeP = MDIS_MA_CHAMELEON;
				*dataModeP = MDIS_MD_CHAM_2;
				*addrSizeP = ADDR_SIZE_SPC_2;
				break;
#endif
			default:
				error = ERR_LL_ILL_PARAM;
			}
			break;
	    }
		/*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
	    }
		/*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_NONE;
			break;
	    }
		/*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
			error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ************************************
 *
 *  Description:  Return ident string
 *
 *---------------------------------------------------------------------------
 *  Input......:  -
 *  Output.....:  return  pointer to ident string
 *  Globals....:  -
 ****************************************************************************/
static char* Ident( void )	/* nodoc */
{
    return( "QSPIM - QSPIM low level driver: $Id: qspim_drv.c,v 2.7 2015/02/19 11:56:46 ts Exp $" );
}

/********************************* Cleanup **********************************
 *
 *  Description: Close all handles, free memory and return error code
 *		         NOTE: The low-level handle is invalid after this function is
 *                     called.
 *
 *---------------------------------------------------------------------------
 *  Input......: h		low-level handle
 *               retCode    return value
 *  Output.....: return	    retCode
 *  Globals....: -
 ****************************************************************************/
static int32 Cleanup(
   LL_HANDLE    *h,
   int32        retCode		/* nodoc */
)
{

	FreeRcvFifo( h );			/* free receive fifo */
#ifdef QSPIM_SUPPORT_A21_DMA
	if( h->recvBuf != NULL )
		OSS_MemFree( h->osHdl, (void *)h->recvBuf, h->recvAlloc );
#endif

	if( h->xmtBuf != NULL )
		OSS_MemFree( h->osHdl, (void *)h->xmtBuf, h->xmtAlloc );

	if( h->frmSig )
		OSS_SigRemove(h->osHdl, &h->frmSig );
	if( h->emgSig )
		OSS_SigRemove(h->osHdl, &h->emgSig );

	if( h->readSemHdl )
		OSS_SemRemove(h->osHdl, &h->readSemHdl);

	/* clean up desc */
	if (h->descHdl)
		DESC_Exit(&h->descHdl);

	/* clean up debug */
	DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(h->osHdl, (int8*)h, h->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
	return(retCode);
}

/********************************* FreeRcvFifo *******************************
 *
 *  Description: Return memory for receive fifo
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: h			low level handle
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void FreeRcvFifo( LL_HANDLE *h )	/* nodoc */
{
	if( h->rcvFifoStart != NULL){
#if defined(LINUX) && defined(QSPIM_SUPPORT_A21_DMA)
		kfree(h->rcvFifoStart);
#else
		OSS_MemFree( h->osHdl, (void *)h->rcvFifoStart, h->rcvFifoAlloc );
#endif

		h->rcvFifoStart = h->rcvFifoEnd = h->rcvFifoNxtIn =
			h->rcvFifoNxtOut = NULL;
		h->rcvFifoAlloc = h->rcvFifoCount = h->rcvFifoDepth = 0;
	}
	if( h->rcvOverBuf != NULL ){
		OSS_MemFree( h->osHdl, (void *)h->rcvOverBuf, h->rcvOverAlloc );
		h->rcvOverBuf = NULL;
	}
}

/********************************* CreateRcvFifo ****************************
 *
 *  Description: Allocate and init the receive fifo
 *
 *			     Free old queue if previously allocated
 *---------------------------------------------------------------------------
 *  Input......: h			low level handle
 *				 depth		number of fifo entries (frames)
 *  Output.....: returns	error code
 *  Globals....: -
 ****************************************************************************/
static int32 CreateRcvFifo( LL_HANDLE *h, int32 depth )	/* nodoc */
{
	u_int32 size;

	FreeRcvFifo(h);			/* free old fifo */

	/*--- allocate memory for fifo ---*/
	size = h->qspiQueueLen * 2 * depth;

#if defined(LINUX) && defined(QSPIM_SUPPORT_A21_DMA)
	h->rcvFifoStart = kmalloc(size, GFP_DMA | GFP_KERNEL);
#else
	h->rcvFifoStart = (u_int8 *)OSS_MemGet( h->osHdl, size, &h->rcvFifoAlloc );
#endif
	if( h->rcvFifoStart == NULL ){

		DBGWRT_ERR((DBH,"*** QSPIM:CreateRcvFifo: couldn't alloc %d bytes\n",
					size ));
		return ERR_OSS_MEM_ALLOC;
	}

	h->rcvFifoEnd = h->rcvFifoStart + size;
	h->rcvFifoNxtIn = h->rcvFifoNxtOut = h->rcvFifoStart;
	h->rcvFifoDepth = depth;
	h->rcvFifoCount = 0;

	/*--- create overrun buffer ---*/
	if( (h->rcvOverBuf =
		(u_int8 *)OSS_MemGet( h->osHdl, h->qspiQueueLen * 2, &h->rcvOverAlloc))
		== NULL ){
		DBGWRT_ERR((DBH,"*** QSPIM:CreateRcvFifo: couldn't alloc %d bytes\n",
					h->qspiQueueLen * 2 ));
		return ERR_OSS_MEM_ALLOC;
	}

	DBGWRT_2((DBH," CreateRcvFifo: depth=%d size=%d\n", depth, size ));
	return 0;
}

/********************************* ClrRcvFifo ********************************
 *
 *  Description: Reset receive fifo
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: h 		low level handle
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void ClrRcvFifo( LL_HANDLE *h ) /* nodoc */
{
	OSS_IRQ_STATE irqState;
	irqState = OSS_IrqMaskR( h->osHdl, h->irqHdl );		/* mask irqs */
	h->rcvFifoCount = 0;
	h->rcvFifoNxtIn = h->rcvFifoNxtOut = h->rcvFifoStart;
	OSS_IrqRestore( h->osHdl, h->irqHdl, irqState );	/* unmask irqs */
}

/********************************* DefineFrm *********************************
 *
 *  Description: Define frame structure.
 *			   	 Called from Setstat QSPIM_BLK_DEFINE_FRM
 *
 *				 Writes configuration into QSPI parameter RAM
 *---------------------------------------------------------------------------
 *  Input......: h			low level handle
 *				 blk		M_SG_BLOCK structure
 *  Output.....: returns:	error code
 *  Globals....: -
 ****************************************************************************/
static int32 DefineFrm( LL_HANDLE *h, M_SG_BLOCK *blk )	/* nodoc */
{
	u_int16 *cfgWordP = (u_int16 *)blk->data;
	u_int32 nCfgWords = blk->size/sizeof(u_int16);
	MACCESS ma = h->maQspi;
	u_int32 cmdOff=0;			/* offset in QPSI command ram */

	DBGWRT_1((DBH,"QSPIM:DefineFrm: nCfgWords=%d\n", nCfgWords ));

	if( blk->size & 1 ){
		DBGWRT_ERR((DBH,"*** QSPIM:DefineFrm: blk->size is not even!\n"));
		return ERR_LL_ILL_PARAM;
	}

	while( nCfgWords-- ){
		u_int8 entries, entry, cmd;

		DBGWRT_2((DBH, " config: 0x%04x\n", *cfgWordP));

		entries = *cfgWordP & 0xff;	/* get number of qspi entries */

		if( cmdOff + entries > h->qspiQueueLen ){
			DBGWRT_ERR((DBH,"*** QSPIM:DefineFrm: too many queue entries\n"));
			return ERR_LL_ILL_PARAM;
		}

		for( entry=0; entry<entries; entry++ ){
			cmd = 0;

			if( entry==0 ){
				/*--- DSCLK - only for first word of slave ---*/
				if( *cfgWordP & 0x1000 )
					cmd |= 0x10;
			}

			if( *cfgWordP & 0x4000 ) 			/* BITSE */
				cmd |= 0x40;

			if( *cfgWordP & 0x2000 ) 			/* DT */
				cmd |= 0x20;

			cmd |= (*cfgWordP & 0x0f00) >> 8 ; 	/* PCS 3..0 */

			if( entry == entries-1 ){
				/* last entry for slave */
				if( *cfgWordP & 0x8000 )		/* CONT */
					cmd |= 0x80;
			}
			else
				cmd |= 0x80;

			DBGWRT_3((DBH,"  cmd=off=0x%02x 0x%02x\n", cmdOff, cmd ));

			MWRITE_D8( ma, QSPI_ECOMDRAM + cmdOff, cmd );
			cmdOff++;
		}
		cfgWordP++;
	}
	h->frmLen = cmdOff*2;		/* save number of bytes used */

	/*--- setup the endq pointer ---*/
	MWRITE_D16( ma, QSPI_SPCR4, (cmdOff-1)<<8 );
#ifdef QSPIM_SUPPORT_A21_DMA
	__DMA_UpdateSize( h );
#endif

	return 0;
}

/********************************* CopyBlock **********************************
 *
 *  Description: A better version of OSS_MemCopy
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: src		source buffer
 *				 dst		destination buffer
 *				 n			number of bytes to copy
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void CopyBlock( u_int32 *src, u_int32 *dst, u_int32 n ) /* nodoc */
{
	u_int8 *src8, *dst8;

	while( n >= 4 ){
		*dst++ = *src++;
		n -= 4;
	}
	if( n ){
		src8 = (u_int8 *)src;
		dst8 = (u_int8 *)dst;

		while(n--)
			*dst8++ = *src8++;
	}
}
