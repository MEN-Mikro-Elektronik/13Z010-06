/****************************************************************************
 ************                                                    ************
 ************                   QSPIM_SIG                        ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: kp
 *        $Date: 2014/07/21 16:05:27 $
 *    $Revision: 1.7 $
 *
 *  Description: Example program for the QSPIM driver
 *				 Uses the signal mode
 *
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: qspim_sig.c,v $
 * Revision 1.7  2014/07/21 16:05:27  ts
 * R: warning about different size at pointer/integer cast
 * M: cast to INT_32_OR_64 instead int32
 *
 * Revision 1.6  2010/05/10 14:21:27  amorbach
 * R: APB build failed
 * M: variable type path changed to MDIS_PATH
 *
 * Revision 1.5  2010/05/06 10:57:07  amorbach
 * R: Porting to MDIS5
 * M: added support for 64bit (MDIS_PATH)
 *
 * Revision 1.4  2010/03/23 11:43:23  apb
 * R: 1. not compilable with current implementation of UOS_Sig
 *    2. build warnings because of type incompatibilities
 * M: 1. added __MAPILIB macro to declaration and implementation of SigHandler
 *    2. fixed types in main and FillFrame()
 *
 * Revision 1.3  2006/03/01 20:49:30  cs
 * cosmetics for MDIS4/2004 compliancy
 *
 * Revision 1.2  2001/04/11 10:23:01  kp
 * setime timer cycle
 * activate FRAMESYN signal
 *
 * Revision 1.1  2000/09/25 13:24:18  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

static const char RCSid[]="$Id: qspim_sig.c,v 1.7 2014/07/21 16:05:27 ts Exp $";

#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/qspim_drv.h>
#include <MEN/usr_oss.h>

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define TIMER 500000			/* low/high time of cycle timer */

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
static u_int16 G_rxFrmBuf[256], G_txFrmBuf[256];
static int32 G_frmSize;
static u_int32 G_xmtErrors;
static u_int32 G_rcvErrors;
static u_int32 G_emgErrors;
static u_int32 G_verErrors;

static u_int32 G_cycles;
static u_int32 G_frmNum;

static u_int32 G_endMe;
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void FillFrame( int32 frameNum );
static void PrintError(char *info);
static void __MAPILIB SigHandler(u_int32 sigCode);


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
	MDIS_PATH path;
	u_int8 i;
	char	*device;
	u_int16 defFrm[16];

	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: qspim_sig <device>\n");
		printf("Function: QSPIM example\n");
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
	UOS_SigInstall( UOS_SIG_USR1 );		/* install signal 1 */
	UOS_SigInstall( UOS_SIG_USR2 );		/* install signal 2 */

	/*--------------------+
    |  config             |
    +--------------------*/

	/* cycle timer */
	if( M_setstat(path, QSPIM_TIMER_CYCLE_TIME, TIMER )) goto abort;

	/* 16 bits per word */
	if( M_setstat(path, QSPIM_BITS, 16 )) goto abort;

	/* baudrate = 6MHz */
	if( M_setstat(path, QSPIM_BAUD, 6000000 )) goto abort;

	/* DSCLK = 2us */
	if( M_setstat(path, QSPIM_DSCLK, 2000 )) goto abort;

	/* ... other configuration may follow */

	/*--- configure QPSI frame ---*/

	/*
	 * 16 slaves/each with 4 words:
	 * activate DSCLK for all slaves, disable DTL
	 */
	blk.data = (void *)defFrm;
	blk.size = 16*2;

	for(i=0; i<16; i++ )
		defFrm[i] = 0xd004 | (i<<8);

	if( M_setstat(path, QSPIM_BLK_DEFINE_FRM, (INT32_OR_64)&blk )) goto abort;

	/*
	 * Fill the tx frame buffer with some data
	 */
	G_frmSize = 64 * 2;			/* set total frame size: 64 words, 128 bytes */

	FillFrame(0);
	G_frmNum = 0;

	/*--- write frame data for very first transfer ---*/
	if( M_setblock( G_path, (u_int8*)G_txFrmBuf, G_frmSize ) < 0 ) goto abort;


	/*--- install signal to be sent when QSPI frame transferred ---*/
	if( M_setstat(path, QSPIM_FRM_SIG_SET, UOS_SIG_USR1 )) goto abort;

	/*--- install signal to be sent when no new data for cycle prepared ---*/
	if( M_setstat(path, QSPIM_EMG_SIG_SET, UOS_SIG_USR2 )) goto abort;


	/*--------------------+
    |  Start Timer/QSPI   |
    +--------------------*/

	/* activate framesync line (next falling SYNCLK edge) */
	if( M_setstat(path, QSPIM_FRAMESYN, TRUE )) goto abort;

	if( M_setstat(path, QSPIM_TIMER_STATE, TRUE )) goto abort;

	/*-------------------------------------+
	|  Let signal handler do the rest...   |
	+-------------------------------------*/
	G_xmtErrors = 0;
	G_rcvErrors = 0;
	G_emgErrors = 0;
	G_cycles = 0;
	G_endMe = 0;

	while( !G_endMe ){
		UOS_Delay(5000);
		printf("cycles=%d xmtErr=%d rcvErr=%d emgErrors=%d verErrors=%d\n",
			   G_cycles, G_xmtErrors, G_rcvErrors, G_emgErrors, G_verErrors );
	}

	/* not reached */
	goto cleanup;

	/*--------------------+
    |  cleanup            |
    +--------------------*/
abort:
	PrintError("config");

cleanup:
	M_setstat(path, QSPIM_FRM_SIG_CLR, 0 );
	M_setstat(path, QSPIM_EMG_SIG_CLR, 0 );

	if (M_close(path) < 0)
		PrintError("close");

	UOS_SigExit();
	return(0);
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
	u_int16 i;

	for(i=0; i<G_frmSize/2; i++ )
		G_txFrmBuf[i] = (u_int16) frameNum+i;
}

/********************************* SigHandler ********************************
 *
 *  Description: Signal handling routine
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: sigCode		signal number
 *  Output.....: -
 *  Globals....: G_xxx
 ****************************************************************************/
static void __MAPILIB SigHandler(u_int32 sigCode)
{
	int32 size, i;

	switch( sigCode ){
	case UOS_SIG_USR1:
		/*--- read entry from receive FIFO ---*/
		if( (size = M_getblock( G_path, (u_int8*)G_rxFrmBuf, G_frmSize ))<0)
			G_rcvErrors++;
		else {
			/*--- compare data with previously sent data ---*/
			for(i=0; i<G_frmSize/2; i++ ){
				if( G_rxFrmBuf[i] != ((G_frmNum+i) & 0xffff) )
					G_verErrors++;
			}
		}
		/*--- write transmit data for next cycle ---*/
		FillFrame(++G_frmNum);
		if( M_setblock( G_path, (u_int8*)G_txFrmBuf, G_frmSize ) < 0 )
			G_xmtErrors++;

		G_cycles++;
		break;
	case UOS_SIG_USR2:
		G_emgErrors++;
		break;
	default:
		M_setstat(G_path, QSPIM_FRM_SIG_CLR, 0 );
		M_setstat(G_path, QSPIM_EMG_SIG_CLR, 0 );
		M_close(G_path);
		G_endMe++;
	}
	return;
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

