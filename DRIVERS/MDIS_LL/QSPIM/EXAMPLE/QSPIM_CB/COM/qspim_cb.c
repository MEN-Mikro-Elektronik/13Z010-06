/****************************************************************************
 ************                                                    ************
 ************                   QSPIM_CB                         ************
 ************                                                    ************
 ****************************************************************************
 *  
 *       Author: kp
 *        $Date: 2010/05/10 14:21:25 $
 *    $Revision: 1.5 $
 *
 *  Description: Example program for the QSPIM driver
 *				 Uses the callback mode.
 *
 *				 !!!! RUNS ONLY UNDER OS-9000 !!!!
 *				 !!!! Compile with -r option (no stack checking) !!!!
 *				 !!!! Compile with -be=pg option (set const pointer) !!!!
 *
 *     Required: libraries: mdis_api, usr_oss, os_men, drvsupp, dbg
 *     Switches: DBG
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
 
static const char RCSid[]="$Id: qspim_cb.c,v 1.5 2010/05/10 14:21:25 amorbach Exp $";


#ifdef OS9000

/*#define DBG						define if debug printfs required */
/*#define PCI_TRACE					define for debug with PCI tracer */

#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/qspim_drv.h>
#include <MEN/usr_oss.h>
#include <MEN/dbg.h>

#include <stddef.h>
#include <stdarg.h>
#include <cglob.h>
#include <regs.h>
#include <errno.h>
#include <MEN/os_men.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define TIMER 500000			/* low/high time of cycle timer */

#define DBG_MYLEVEL				DBG_ALL
#define DBH 					G_dbh

#define DPC_BASE_VECTOR 		0x50
#define DPC_NUM 				9 /* must match the interrupt number of PIC */

#ifdef PCI_TRACE
# define _TRACE(x) *(u_int16 *)0xe1000004 = x
#else
# define _TRACE(x) {}
#endif

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static MDIS_PATH G_path;
static u_int16 G_txFrmBuf[256];
static int32   G_frmSize;
static volatile u_int32 G_xmtErrors;
static volatile u_int32 G_rcvErrors;
static volatile u_int32 G_emgErrors;
static volatile u_int32 G_verErrors;
static volatile u_int32 G_dpcs;
static volatile u_int32 G_cycles;
static volatile u_int32 G_frmNum;
static volatile u_int32 G_abort;

QSPIM_DIRECT_WRITE_FUNCP G_directWriteFunc;
QSPIM_DIRECT_ISETSTAT_FUNCP G_directISetstatFunc;

void *G_directWriteArg;
void *G_directISetstatArg;
DBG_HANDLE *G_dbh;

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void PrintError(char *info);
static void __MAPILIB SigHandler(u_int32 sigCode);
void Callback( void *arg,u_int16 *qspiFrame, u_int32 qspiFrameLen );
u_int32 CallbackDpc(void);
static void FillFrame( int32 frameNum );


/********************************* main *************************************
 *
 *  Description: Program main function
 *			   
 *---------------------------------------------------------------------------
 *  Input......: argc,argv	argument counter, data ..
 *  Output.....: return	    success (0) or error (1)
 *  Globals....: -
 ****************************************************************************/
int main(int argc, char *argv[])
{
	M_SG_BLOCK blk;
	MDIS_PATH	path;
	int32	i;
	char	*device;
	u_int16 defFrm[16];

	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: qspim_cb <device>\n");
		printf("Function: QSPIM example for callback mode\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("\n");
		return(1);
	}
	
	device = argv[1];

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((G_path = path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/* get ready to handle signals */
	UOS_SigInit( SigHandler );			/* init signal handling */
	UOS_SigInstall( UOS_SIG_USR2 );		/* install signal 2 */

	/*--------------------+
    |  config             |
    +--------------------*/

	/* 16 bits per word */
	if( M_setstat(path, QSPIM_BITS, 16 )) goto abort;

	/* baudrate = 6MHz */
	if( M_setstat(path, QSPIM_BAUD, 6000000 )) goto abort;

	/* DSCLK = 2us */
	if( M_setstat(path, QSPIM_DSCLK, 1000 )) goto abort;
	
   	/* cycle timer */
	if( M_setstat(path, QSPIM_TIMER_CYCLE_TIME, TIMER )) goto abort;

	/* ... other configuration may follow */

	/*--- configure QPSI frame ---*/
	
	/* 
	 * 18 slaves/each with 4 words: 
	 * activate DSCLK for all slaves, disable DTL
	 */
	blk.data = (void *)defFrm;
	blk.size = 18*2;

	for(i=0; i<18; i++ )
		defFrm[i] = 0xd004 | ((i&0xf)<<8);

	if( M_setstat(path, QSPIM_BLK_DEFINE_FRM, (int32)&blk )) goto abort;

	/*
	 * Fill the tx frame buffer with some data
	 */
	G_frmSize = 72 * 2;			/* set total frame size: 72 words, 144 bytes */
	
	FillFrame(0);				
	G_frmNum = 0;

	/*--- write frame data for very first transfer ---*/
	if( M_setblock( G_path, (u_int8*)G_txFrmBuf, G_frmSize ) < 0 ) goto abort;


	/*--- install signal to be sent when no new data for cycle prepared ---*/
	if( M_setstat(path, QSPIM_EMG_SIG_SET, UOS_SIG_USR2 )) goto abort;

	/*--- install callback function ---*/
	{
		QSPIM_CALLBACK_PARMS p;

		blk.data = (void *)&p;
		blk.size = sizeof(p);

		p.func = Callback;
		p.arg  = (void *)0x12345678;	/* just for testing */
		p.statics = _glob_data;

		if( M_setstat(path, QSPIM_BLK_CALLBACK, (int32)&blk )) goto abort;
	}
	
	/*--- query direct write function ---*/
	{
		QSPIM_DIRECT_WRITE_PARMS p;

		blk.data = (void *)&p;
		blk.size = sizeof(p);

		if( M_getstat(path, QSPIM_BLK_DIRECT_WRITE_FUNC, (int32*)&blk )) 
			goto abort;

		G_directWriteFunc = p.func;
		G_directWriteArg  = p.arg;
	}
	
	/*--- query direct isetstat function ---*/
	{
		QSPIM_DIRECT_ISETSTAT_PARMS p;

		blk.data = (void *)&p;
		blk.size = sizeof(p);

		if( M_getstat(path, QSPIM_BLK_DIRECT_ISETSTAT_FUNC, (int32*)&blk )) 
			goto abort;

		G_directISetstatFunc = p.func;
		G_directISetstatArg  = p.arg;
	}
	
	/*--- prepare for debug functions ---*/
	DBGINIT((NULL,&DBH));


	/*--- install handler for defered procedure call ---*/
	if( errno = _osmen_irq( DPC_BASE_VECTOR+DPC_NUM, 2, 
							(void *)CallbackDpc, _glob_data ))
		goto abort;

	printf("starting. globs at 0x%x\n", _glob_data);
	UOS_Delay(1000);

	/*--------------------+
    |  Start Timer/QSPI   |
    +--------------------*/

	/* activate framesync line (next falling SYNCLK edge) */
	if( M_setstat(path, QSPIM_FRAMESYN, TRUE )) goto abort;

	if( M_setstat(path, QSPIM_TIMER_STATE, TRUE )) goto abort;


	/*-------------------------------------+
	|  Let callback do the rest...         |
	+-------------------------------------*/
	G_xmtErrors = 0;
	G_rcvErrors = 0;
	G_emgErrors = 0;
	G_cycles = 0;
	G_dpcs = 0;
	G_abort = FALSE;

	while(G_abort == FALSE){
		UOS_Delay(1000);
		printf("cycles=%d dpcs=%d xmtErr=%d rcvErr=%d emgErrors=%d "
			   "verErrors=%d\n", 
			   G_cycles, G_dpcs, G_xmtErrors, G_rcvErrors, G_emgErrors,
			   G_verErrors);
	}

	goto cleanup;

	/*--------------------+
    |  cleanup            |
    +--------------------*/
abort:
	PrintError("config");
	
cleanup:	
	M_setstat(G_path, QSPIM_EMG_SIG_CLR, 0 );
	M_setstat(path, QSPIM_FRAMESYN, FALSE );
	UOS_Delay(10);
	M_setstat(G_path, QSPIM_TIMER_STATE, FALSE );
	UOS_Delay(10);

	{
		QSPIM_CALLBACK_PARMS p;

		blk.data = (void *)&p;
		blk.size = sizeof(p);

		p.func = NULL;
		p.arg  = NULL;

		M_setstat(G_path, QSPIM_BLK_CALLBACK, (int32)&blk );
	}

	_osmen_irq( DPC_BASE_VECTOR+DPC_NUM, 2, NULL, _glob_data );

	M_close(G_path);

	UOS_SigExit();

	return(0);
}

/********************************* Callback ***********************************
 *
 *  Description: Callback routine for QPSI transfer finished
 *			   	 
 *	!!! This code is executed in system state !!!		   
 *---------------------------------------------------------------------------
 *  Input......: arg		argument that has been passed during installation
 *				 qspiFrame	ptr to QSPI received frame data
 *				 qspiFrameLen number of bytes in QSPI frame
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
void Callback( void *arg,u_int16 *qspiFrame, u_int32 qspiFrameLen )
{
	int32 i;

	_TRACE(0x1111);
	DBGWRT_1((DBH,"Callback arg=0x%x\n", arg));
	
	G_cycles++;

	/*--- compare data with previously sent data ---*/
	for(i=0; i<qspiFrameLen/2; i++ ){
		if( qspiFrame[i] != ((G_frmNum+i) & 0xffff) )
			G_verErrors++;
	}

	_TRACE(0x1112);
	_osmen_trigger_dpc(DPC_NUM); /* trigger my DPC (CallbackDpc) */
	_TRACE(0x1113);
}	

/********************************* CallbackDpc *******************************
 *
 *  Description: Defered routine triggered by Callback
 *			   
 *  This routine may be interrupted only by higher priority interrupts or
 *  by the interrupt that triggered that DPC.
 *
 *	!!! This code is executed in system state !!!		   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: -
 *  Output.....: returns: always E_NOTME
 *  Globals....: -
 ****************************************************************************/
u_int32 CallbackDpc(void)
{
	_TRACE(0x2222);
	G_dpcs++;

#if 0
	/* perform some dummy computations */
	{
		volatile u_int32 i;

		for(i=0; i<20; i++ )
			_TRACE(0x2000+i);
	}
#endif
	/*--- send data for next frame ---*/
	FillFrame( ++G_frmNum );
	if( G_directWriteFunc( G_directWriteArg, G_txFrmBuf, G_frmSize ))
		G_xmtErrors++;

	/*--- toggle FRAMESYNC every 100th DPC ---*/
	if( (G_dpcs % 100) == 0 )
		G_directISetstatFunc( G_directISetstatArg, QSPIM_FRAMESYN, 0, 
							  !!(G_dpcs % 200));


	_TRACE(0x2223);
	return E_NOTME;
}

/********************************* SigHandler ********************************
 *
 *  Description: Signal handling routine
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: 
 *  Output.....: 
 *  Globals....: -
 ****************************************************************************/
static void __MAPILIB SigHandler(u_int32 sigCode)
{
	switch( sigCode ){
	case UOS_SIG_USR2:
		G_emgErrors++;			/* DPC too slow */
		break;
	default:
		G_abort = TRUE;
		break;
	}
}

/********************************* FillFrame *********************************
 *
 *  Description: Fill the Tx frame buffer with test pattern
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: frmNum		frame number (first frame=0, second=1...)
 *  Output.....: -
 *  Globals....: G_frmSize, G_txFrmBuf
 ****************************************************************************/
static void FillFrame( int32 frameNum )
{
	int32 i;

	for(i=0; i<G_frmSize/2; i++ )
		G_txFrmBuf[i] = frameNum+i;
}

/********************************* PrintError *******************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}


/*
 * :-( must declare my own printf here, because printf and DBG_Write needs
 * vsprintf, but vsprintf can't be used from clib, since it is compiled
 * with stackchecking code
 */
int printf( const char *fmt, ... )
{
	char buf[512];
	va_list argptr;
	u_int32 len;

	va_start(argptr,fmt);	
	vsprintf(buf, fmt, argptr );
	va_end(argptr);

	len = strlen(buf);
	_os_writeln( 1, buf, &len);
	return len;
}


#endif /* OS9000 */



