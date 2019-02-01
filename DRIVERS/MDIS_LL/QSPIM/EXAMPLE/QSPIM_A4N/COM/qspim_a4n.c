/****************************************************************************
 ************                                                    ************
 ************                   QSPIM_A4N                        ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: kp
 *        $Date: 2014/07/21 16:04:46 $
 *    $Revision: 1.6 $
 *
 *  Description: Example program for the QSPIM driver
 *				 To be used with A4N card and special connection.
 *
 *				SCLK -> A4N IC45.2     100R series resistor
 *				MOSI -> A4N IC45.3     100R series resistor
 *				MISO -> A4N M-Mod0 C16 100R series resistor
 *				PCS0 -> A4N IC44.6	   100R series resistor
 *
 *              A4N QSPI setup:
 *				FFFC16: 7b71
 *				FFFC18: 0100
 *				FFFC1C: 4F00
 *				FFFC1A:	8000
 *
 *				D FFFD00 to view data
 *
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2010 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
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

#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/qspim_drv.h>
#include <MEN/usr_oss.h>

#include <stddef.h>
/* #include <sysglob.h> */			/* ??? */
/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* none */

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

static u_int32 G_cycles;
static u_int32 G_endMe;
/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
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
	MDIS_PATH	path;
	u_int16 i;
	char	*device;
	u_int16 defFrm[2];

	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: qspim_a4n <device>\n");
		printf("Function: QSPIM example using A4N\n");
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
	printf("*** qspim_sig: device opened\n");

	/* get ready to handle signals */
	UOS_SigInit( SigHandler );			/* init signal handling */
	UOS_SigInstall( UOS_SIG_USR1 );		/* install signal 1 */
	UOS_SigInstall( UOS_SIG_USR2 );		/* install signal 2 */
	printf("*** qspim_sig: sigs installed\n");

	/*--------------------+
    |  config             |
    +--------------------*/

	/* 16 bits per word */
	if( M_setstat(path, QSPIM_BITS, 16 )) goto abort;

	/* baudrate = 4MHz */
	if( M_setstat(path, QSPIM_BAUD, 4000000 )) goto abort;

	/* DSCLK = 2us */
	if( M_setstat(path, QSPIM_DSCLK, 2000 )) goto abort;

	/* DTL = 1us, must be used for A4N, otherwise we're too fast */
	if( M_setstat(path, QSPIM_DTL, 1000 )) goto abort;

	if( M_setstat(path, QSPIM_CPHA, 1 )) goto abort;

	printf("*** qspim_sig: setup finished\n");

	/*--- configure QPSI frame ---*/

	/*
	 * two slaves:
	 * activate DSCLK for all slaves, disable DTL
	 * slave addr 0 (A4N):  	16 words
	 * slave addr 1 (nothing):  4 words
	 */
	blk.data = (void *)defFrm;
	blk.size = sizeof(defFrm);

	defFrm[0] = 0xf010;			/* slave 0 */
	defFrm[1] = 0xf104;			/* slave 1 */


	if( M_setstat(path, QSPIM_BLK_DEFINE_FRM, (INT32_OR_64)&blk )) goto abort;

	/*
	 * Fill the tx frame buffer with some data
	 */
	G_frmSize = 20 * 2;			/* set total frame size: 20 words, 40 bytes */

	for( i=0; i<16; i++ )
		G_txFrmBuf[i] = i;


	/*--- write frame data for very first transfer ---*/
	if( M_setblock( G_path, (u_int8*)G_txFrmBuf, G_frmSize ) < 0 ) goto abort;

	/*--- install signal to be sent when QSPI frame transferred ---*/
	if( M_setstat(path, QSPIM_FRM_SIG_SET, UOS_SIG_USR1 )) goto abort;

	/*--- install signal to be sent when no new data for cycle prepared ---*/
	if( M_setstat(path, QSPIM_EMG_SIG_SET, UOS_SIG_USR2 )) goto abort;


	/*--------------------+
    |  Start Timer/QSPI   |
    +--------------------*/
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
		UOS_Delay(1000);
		printf("CY %10d XE=%d RE=%d EE=%d ",
			   (int)G_cycles, (int)G_xmtErrors, (int)G_rcvErrors, (int)G_emgErrors );

		printf("Data: ");
		for(i=0; i<16; i++ )
			printf("%04x ", G_rxFrmBuf[i] );
		printf("\n");
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
	int32 size, i;

	switch( sigCode ){
	case UOS_SIG_USR1:
		/*--- read entry from receive FIFO ---*/
		if( (size = M_getblock( G_path, (u_int8*)G_rxFrmBuf, G_frmSize ))<0)
			G_rcvErrors++;

		for( i=0; i<16; i++ )	/* increase all data words */
			G_txFrmBuf[i]++;

		/*--- write transmit data for next cycle ---*/
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

