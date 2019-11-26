/****************************************************************************
 ************                                                    ************
 ************                   QSPIM_XC7                        ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: ag
 *
 *  Description: Example program for the QSPI-Interface on XC7
 *				 Uses the signal mode
 *
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright 2010-2019, MEN Mikro Elektronik GmbH
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

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define NUM_ADC_CAHNNELS  4
#define NUM_RX_TX_BYTES   2

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
static u_int16   G_rxFrmBuf;
static u_int16   G_txFrmBuf[5];
static int32     G_frmSize;
static u_int32   G_emgErrors;

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
	MDIS_PATH path;
	int32   result, k;
	char   *device;
	u_int16 defFrm[5];
 	u_int16 curFrm = 0;
	u_int8  bitWidth[5];
	u_int8  cpol[5];
	u_int8  cpha[5];
	u_int32 baud[5];

	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: qspim_xc7 <device>\n");
		printf("Function: XC7 QSPIM example\n");
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

	/* config QSPI */
	printf("Setup QSPIM");
	/* cycle timer */
	if( M_setstat(path, QSPIM_TIMER_CYCLE_TIME, 1600000 )){
		PrintError("QSPIM_TIMER_CYCLE_TIME");
		M_close(path);
		return(1);
	}
	printf(".");
	/* try with 16 bits per word */
	if( M_setstat(path, QSPIM_BITS, 16 )){
		PrintError("QSPIM_BITS");
		M_close(path);
		return(1);
	}
	printf(".");
	/* try with baudrate = 5MHz(6MHz) */
	if( M_setstat(path, QSPIM_BAUD, 8000000 )) {
		PrintError("QSPIM_BAUD");
		M_close(path);
		return(1);
	}
	printf(".");
	/* DSCLK = 2us */
	if( M_setstat(path, QSPIM_DSCLK, 2000 )) {
		PrintError("QSPIM_DSCLK");
		M_close(path);
		return(1);
	}
	printf(".\n");

	/*--- install signal to be sent when QSPI frame transferred ---*/
	if( M_setstat(path, QSPIM_FRM_SIG_SET, UOS_SIG_USR1 )) goto abort;

	/*--- install signal to be sent when no new data for cycle prepared ---*/
	if( M_setstat(path, QSPIM_EMG_SIG_SET, UOS_SIG_USR2 )) goto abort;


	/* baudrate */
	baud[0] = 2000000;
	baud[1] = 8000000;
	baud[2] = 8000000;
	baud[3] = 8000000;
	baud[4] = 8000000;

	/* clock polarity */
	cpol[0] = 0;
	cpol[1] = 1;
	cpol[2] = 1;
	cpol[3] = 1;
	cpol[4] = 1;

	/* clock phase */
	cpha[0] = 1;
	cpha[1] = 1;
	cpha[2] = 1;
	cpha[3] = 1;
	cpha[4] = 1;

	/* bit width */
	bitWidth[0] = 8;
	bitWidth[1] = 16;
	bitWidth[2] = 16;
	bitWidth[3] = 16;
	bitWidth[4] = 16;

	/*
	 * two slaves:
	 * activate BITSE, DT, DSCLK for all slaves, disable CONT (0x7...)
	 * Use /CS0 for Low Side Switch                           (0x.e..)
	 * Use /CS1 for ADC                                       (0x.d..)
	 * Use single word transfer                               (0x..01)
	 */

	blk.data = (void *)&curFrm;
	blk.size = G_frmSize = sizeof(curFrm);

	defFrm[0] = 0x7e01;		/* Command forLSS,Diag */
	defFrm[1] = 0x7d01;		/* ADC,CH1  */
	defFrm[2] = 0x7d01;		/* ADC,CH2  */
	defFrm[3] = 0x7d01;		/* ADC,CH3  */
	defFrm[4] = 0x7d01;		/* ADC,CH4  */

	G_txFrmBuf[0] = 0x0000;	/* LSS, Diag command */
	G_txFrmBuf[1] = 0x0800;	/* ADC sel CH2 */
	G_txFrmBuf[2] = 0x1000;	/* ADC sel CH3 */
	G_txFrmBuf[3] = 0x1800;	/* ADC sel CH4 */
	G_txFrmBuf[4] = 0x0000;	/* ADC sel CH1 */

	UOS_Delay(1);

	/*-------------------------------------+
	|  Let signal handler do the rest...   |
	+-------------------------------------*/
	G_emgErrors = 0;

	for( k = 0; k < 5; k++ ){

		/* set clock polarity */
		if( M_setstat(path, QSPIM_CPOL, cpol[k] )){
			PrintError("QSPIM_CPOL");
			goto cleanup;
		}

		/* set clock phase */
		if( M_setstat(path, QSPIM_CPHA, cpha[k] )){
			PrintError("QSPIM_CPHA");
			goto cleanup;
		}

		/* set baudrate */
		if( M_setstat(path, QSPIM_BAUD, baud[k] )) {
			PrintError("QSPIM_BAUD");
			goto cleanup;
		}

		/* set bitwidth */
		if( M_setstat(path, QSPIM_BITS, bitWidth[k] )){
			PrintError("QSPIM_BITS");
			goto cleanup;
		}

		curFrm = defFrm[k];
		if( M_setstat(path, QSPIM_BLK_DEFINE_FRM, (INT32_OR_64)&blk )) {
			PrintError("QSPIM_BLK_DEFINE_FRM");
			goto cleanup;
		}
		/* write command */
		result = M_setblock( path, (u_int8*)&G_txFrmBuf[k], NUM_RX_TX_BYTES );
		if(  result  < 0 ) {
			PrintError("M_setblock");
			goto cleanup;
		}

		/*--------------------+
		|  Start Timer/QSPI   |
		+--------------------*/
		/* activate framesync line (next falling SYNCLK edge) */
		if( M_setstat(path, QSPIM_FRAMESYN, TRUE )) goto abort;
		if( M_setstat(path, QSPIM_TIMER_STATE, TRUE )) goto abort;

		UOS_Delay(1000);
		if( k ) {
			u_int8 adcVal = (u_int8) ((G_rxFrmBuf >> 4) & 0x00ff);
			float adcVoltage;
			adcVoltage = (float)((3.388 * adcVal) / 255);
			printf("EmgErrors=%d rxData[0]=0x%04X ADC-Value=0x%02X ADC-Voltage=%02.2fV IN-Voltage=%02.2fV\n",
					G_emgErrors, G_rxFrmBuf, adcVal, adcVoltage, (11 * adcVoltage) );
		}
		else {
			printf("EmgErrors=%d rxData[0]=0x%04X LSS-Diag=0x%02X\n",
				   G_emgErrors, G_rxFrmBuf, (u_int8) (G_rxFrmBuf & 0x00ff) );
		}
	} /* for */

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
 *  Input......: sigCode		signal number
 *  Output.....: -
 *  Globals....: G_xxx
 ****************************************************************************/
static void __MAPILIB SigHandler(u_int32 sigCode)
{

	switch( sigCode ){
	case UOS_SIG_USR1:
		/*--- read entry from receive FIFO ---*/
		M_getblock( G_path, (u_int8*)&G_rxFrmBuf, NUM_RX_TX_BYTES );

		M_setstat(G_path, QSPIM_FRAMESYN, FALSE );
		M_setstat(G_path, QSPIM_TIMER_STATE, FALSE );

		break;
	case UOS_SIG_USR2:
		G_emgErrors++;
		break;
	default:
		M_setstat(G_path, QSPIM_FRM_SIG_CLR, 0 );
		M_setstat(G_path, QSPIM_EMG_SIG_CLR, 0 );
		M_close(G_path);
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
