/****************************************************************************
 ************                                                    ************
 ************                   QSPIM_BC02                       ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: dp
 *
 *  Description: Example program for the QSPIM driver
 *
 *				 To configure the Micrel KS8995MA/FQ ethernet switch at BC02.
 *
 *               !!! Note: BC02 GPIO 2 bit7 (mux_if1) must be set to 1 !!!  
 *
 *     Required: libraries: mdis_api, usr_oss
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2012-2019, MEN Mikro Elektronik GmbH
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
#include <stdlib.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/qspim_drv.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* Micrel KS8995MA/FQ */
#define KS_BAUDRATE		5000000	/* max. 5MHz */
#define KS_WORD_SIZE	8		/* 8 bits per word */
#define KS_CPOL			0		/* base value of the clock is zero */
#define KS_CPHA			0		/* data captured on rising edge, data propagated on falling edge */
#define KS_DSCLK		200		/* tCHSL >90ns */
#define KS_DTL			200		/* tSHSL >100ns */
#define KS_REG_NBR		110		/* register 0x00..0x6d */
#define KS_MAX_WORDS	(2+KS_REG_NBR)	/* cmd, addr, all regs */

#define CMD_READ		0x03
#define CMD_WRITE		0x02

#define CMD		0
#define ADDR	1
#define DATA	2

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
typedef enum {
	none,
	qspread,
	qspwrite
} SINGLE_RW_TYPE;

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static u_int16 G_cfgFrmBuf[KS_REG_NBR] = {
/*
0     1  	 2     3     4     5     6     7     8     9     a     b     c     d     e     f 
*/
/* 0x00..0x0f --- global */
0x95, 0x00, 0x4c, 0x8d, 0xfa, 0x00, 0x00, 0x5a, 0x24, 0x24, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00,
/* 0x10..0x1f --- port 1 */
0x00, 0x1f, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 
/* 0x20..0x2f --- port 2 */
0x00, 0x1f, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 
/* 0x30..0x3f --- port 3 */
0x00, 0x1f, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 
/* 0x40..0x4f --- port 4 */
0x00, 0x1f, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 
/* 0x50..0x5f --- port 5 */
0x00, 0x1f, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 
/* 0x60..0x6d --- global */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0xa1, 0xff, 0xff, 0xff
/* 0x6e..0x78 --- global, indirect access registers */
/* 0x79..0x7f --- global, do not r/w to these registers (Micrel internal testing only) */
};

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void usage(void);
static void RegDump(u_int16 *ptr);
static void PrintError(char *info);
static void PrintUosError(char *info);

/********************************* usage ***********************************/
/**  Print program usage
 */
static void usage(void)
{
	printf("Usage: qspim_bc02 [<opts>] <device> [<opts>]\n");
	printf("Function: QSPIM example for ethernet switch at BC02\n");
	printf("Options:\n");
	printf("    device              device name                          [none]\n");
	printf("    --- Single Transfer ---\n");
	printf("    -r=<reg>            read from register <reg>             [none]\n");
	printf("    -r=<reg> -v=<byte>  write <byte> to register <reg>       [none]\n");
	printf("    -d                  dump all regsiters                   [none]\n");
	printf("    -c                  configure switch                     [none]\n");
	printf("    --- Block Transfer ---\n");
	printf("    -d -b               dump all regsiters                   [none]\n");
	printf("    -c -b               configure switch                     [none]\n");
	printf("\n");
	printf("Notes:\n");
	printf("- <reg> = 0..%d or 0x00..0x%x\n", KS_REG_NBR-1, KS_REG_NBR-1);
	printf("- BC02 GPIO 2 bit7 (mux_if1) must be set to 1\n");
	printf("\n");
	printf("Example:\n");
	printf("1) stop switch   : qspim_bc02 <device> -r=0x01 -v=0x00\n");
	printf("2) config switch : qspim_bc02 <device> -c\n");
	printf("3) start switch  : qspim_bc02 <device> -r=0x01 -v=0x01\n");
	printf("\n");
	printf("Copyright (c) 2012-2019, MEN Mikro Elektronik GmbH\n%s\n", IdentString);
}

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
	M_SG_BLOCK		blk;
	MDIS_PATH		path;
    u_int16			i;
	char			*device, *str, *errstr, buf[40];
	int32			reg, val;
	int32			n, dump, cfg, size, block;
	u_int32			sigCode;
	u_int16			defFrm;
	u_int16			txFrmBuf[3];
	u_int16			rxFrmBuf[3];
	u_int16			txBlkFrmBuf[KS_MAX_WORDS];
	u_int16			rxBlkFrmBuf[KS_MAX_WORDS];
	u_int8			trxNbr;
	SINGLE_RW_TYPE	singleRw;

	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if( (errstr = UTL_ILLIOPT("r=v=dcb?", buf)) ){	/* check args */
		printf("*** %s\n", errstr);
		return(1);
	}

	if( UTL_TSTOPT("?") ){						/* help requested ? */
		usage();
		return(1);
	}

	/*--------------------+
    |  get arguments      |
    +--------------------*/
	for (device=NULL, n=1; n<argc; n++)
		if( *argv[n] != '-' ){
			device = argv[n];
			break;
		}

	if( !device) {
		usage();
		return(1);
	}

	if( (str = UTL_TSTOPT("r=")) ){

		/* hex? */
		if( 0 == strncmp( str, "0x", 2 ))
			sscanf( str, "%x", &reg );
		/* dec */
		else
			reg = (char)atoi(str);

		if( reg >= KS_REG_NBR ){
			printf("*** -r=0x%x out of range\n", reg); 
			usage();
			return(1);
		}
		singleRw = qspread;
	}
	else {
		singleRw = none;
	}

	if( (str = UTL_TSTOPT("v=")) ){

		/* hex? */
		if( 0 == strncmp( str, "0x", 2 ))
			sscanf( str, "%x", &val );
		else
			val = (char)atoi(str);

		if( singleRw == none ){
			printf("*** -v=0x%x requires -r=<reg>\n", val); 
			usage();
			return(1);
		}
		singleRw = qspwrite;	
	}

	dump = (UTL_TSTOPT("d") ? 1 : 0);
	cfg  = (UTL_TSTOPT("c") ? 1 : 0);
	block  = (UTL_TSTOPT("b") ? 1 : 0);

	if( (dump || cfg) && (singleRw != none) ){
		printf("*** single and block access cannot be used at once\n"); 
		usage();
		return(1);
	}

	if( block && singleRw ){
		printf("*** -b only for -d/-c\n"); 
		usage();
		return(1);
	}

	/* nothing to do? */
	if( !(dump || cfg || (singleRw != none)) ){
		usage();
		return(1);
	}

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("M_open");
		return(1);
	}

	/* init signal handling */
	if( UOS_SigInit(NULL)) {
		PrintUosError("UOS_SigInit");
		goto cleanup;
	}	

	/* install signal 1 */
	if( UOS_SigInstall(UOS_SIG_USR1)) {
		PrintUosError("UOS_SigInstall(UOS_SIG_USR1)");
		goto cleanup;
	}		

	/*--------------------+
    |  config             |
    +--------------------*/
	/* prevent frame duplication */
	if( M_setstat(path, QSPIM_NO_DUPLICATION, 1 )) {
		PrintError("M_setstat(QSPIM_NO_DUPLICATION)");
		goto cleanup;
	}

	/* set word size */
	if( M_setstat(path, QSPIM_BITS, KS_WORD_SIZE )) {
		PrintError("M_setstat(QSPIM_BITS)");
		goto cleanup;
	}

	/* set cycle timer (to slowest possible value) */
	if( M_setstat(path, QSPIM_TIMER_CYCLE_TIME, 1600000 )) {
		PrintError("M_setstat(QSPIM_TIMER_CYCLE_TIME)");
		goto cleanup;
	}

	/* set baudrate */
	if( M_setstat(path, QSPIM_BAUD, KS_BAUDRATE )) {
		PrintError("M_setstat(QSPIM_BAUD)");
		goto cleanup;
	}

	/* set clock polarity */
	if( M_setstat(path, QSPIM_CPOL, KS_CPOL )) {
		PrintError("M_setstat(QSPIM_CPOL)");
		goto cleanup;
	}

	/* set clock phase  */
	if( M_setstat(path, QSPIM_CPHA, KS_CPHA )) {
		PrintError("M_setstat(QSPIM_CPHA)");
		goto cleanup;
	}

	/* set default state of PCS3..0 */
	if( M_setstat(path, QSPIM_PCS_DEFSTATE, 0xf )) {
		PrintError("M_setstat(QSPIM_PCS_DEFSTATE)");
		goto cleanup;
	}

	/* set PCS to SCLK delay */
	if( M_setstat(path, QSPIM_DSCLK, KS_DSCLK )) {
		PrintError("M_setstat(QSPIM_DSCLK)");
		goto cleanup;
	}

	/* set delay after transfer */
	if( M_setstat(path, QSPIM_DTL, KS_DTL )) {
		PrintError("M_setstat(QSPIM_DTL)");
		goto cleanup;
	}

	/* install signal to be sent when QSPI frame transferred */
	if( M_setstat(path, QSPIM_FRM_SIG_SET, UOS_SIG_USR1 )) {
		PrintError("M_setstat(QSPIM_FRM_SIG_SET)");
		goto cleanup;
	}

	/*
	 * configure QSPI frame 
	 *
     *	Bit 15    14    13    12     11    10    9     8       7 .. 0
     *	--------------------------------------------------------------
     *      CONT  BITSE DT    DSCLK  PCS3  PCS2  PCS1  PCS0    NWORDS
	 *
	 * Activate BITSE, DT, DSCLK for all slaves, disable CONT (0x7...)
	 * Use /CS0 for switch                                    (0x.e..)
	 * Use transfer with  3  words (cmd + addr +  1  values)  (0x..03)
	 *          KS_MAX_WORDS words        KS_REG_NBR values   (0x..KS_MAX_WORDS)
	 */
	defFrm = 0x7e00;
	if( block )
		defFrm |= KS_MAX_WORDS;
	else
		defFrm |= 0x03;

	blk.data = (void*)&defFrm;
	blk.size = sizeof(defFrm);
	if( M_setstat(path, QSPIM_BLK_DEFINE_FRM, (INT32_OR_64)&blk )) {
		PrintError("M_setstat(QSPIM_BLK_DEFINE_FRM)");
		goto cleanup;
	}

	/*------------------------+
	|  Block Transfer         |
	+------------------------*/
	if( block ){
		printf("Block Transfer: 1 frame with %d words (cmd + addr + %d values)\n",
			KS_MAX_WORDS, KS_REG_NBR);

		/* block read */
		if( dump ){
			txBlkFrmBuf[0] = CMD_READ;	/* command */
			/* data (unused) */
			memset( (void*)&txBlkFrmBuf[2], 0x00, sizeof(G_cfgFrmBuf) ); 
		}
		/* block write */
		else{
			txBlkFrmBuf[0] = CMD_WRITE;	/* command */
			/* data for configuration */
			memcpy( (void*)&txBlkFrmBuf[2], (void*)G_cfgFrmBuf, sizeof(G_cfgFrmBuf) );
		}
		txBlkFrmBuf[1] = 0x00;			/* address */

		/* write frame data */
		if( M_setblock( path, (u_int8*)txBlkFrmBuf, sizeof(txBlkFrmBuf) ) < 0 ) {
			PrintError("M_setblock");
			goto cleanup;
		}

		/* start timer/QSPI */
		if( M_setstat(path, QSPIM_TIMER_STATE, TRUE )) {
			PrintError("M_setstat(QSPIM_TIMER_STATE)");
			goto cleanup;
		}

		/* wait for transfer done */
		do {
			if( UOS_SigWait( 1000, &sigCode )) {
				PrintUosError("UOS_SigWait");
				goto cleanup;
			}	
		} while( sigCode != UOS_SIG_USR1 );

		/* read entry from receive FIFO */
		if( (size = M_getblock( path, (u_int8*)rxBlkFrmBuf, sizeof(rxBlkFrmBuf) ))<0) {
			PrintError("M_getblock");
			goto cleanup;
		}
	}
	/*------------------------+
	|  Single Transfer        |
	+------------------------*/
	else {
		/* all regs? */
		if( singleRw == none ){
			printf("Single Transfer: %d frames with 3 words (cmd + addr + value)\n",
				KS_REG_NBR);
			trxNbr = KS_REG_NBR;

			/* read */
			if( dump ){
				txFrmBuf[CMD] = CMD_READ;	/* command */
				txFrmBuf[DATA] = 0x00;		/* data (unused) */
			}
			/* write */
			else{
				txFrmBuf[CMD] = CMD_WRITE;	/* command */
			}
		}
		/* one reg */
		else {
			printf("Single Transfer: 1 frame with 3 words (cmd + addr + value)\n");
			trxNbr = 1;

			/* read */
			if( singleRw == qspread ){
				txFrmBuf[CMD] = CMD_READ;	/* command */
				txFrmBuf[DATA] = 0x00;		/* data (unused) */
			}
			/* write */
			else {
				txFrmBuf[CMD] = CMD_WRITE;		/* command */
				txFrmBuf[DATA] = (u_int16)val;	/* data */
			}

			txFrmBuf[ADDR] = (u_int16)reg;		/* address */
		}

		rxFrmBuf[CMD] = 0x00;	/* unused */
		rxFrmBuf[DATA] = 0x00;	/* unused */

		for( i=0; i<trxNbr; i++){

			/* all regs? */
			if( singleRw == none ){
				txFrmBuf[ADDR] = i;			/* address */

				/* write */
				if( !dump ){
					txFrmBuf[DATA] = G_cfgFrmBuf[i];	/* data */
				}
			}

			/* write frame data */
			if( M_setblock( path, (u_int8*)txFrmBuf, sizeof(txFrmBuf) ) < 0 ) {
				PrintError("M_setblock");
				goto cleanup;
			}

			/* first frame? */
			if( i == 0){
				/* start timer/QSPI */
				if( M_setstat(path, QSPIM_TIMER_STATE, TRUE )) {
					PrintError("M_setstat(QSPIM_TIMER_STATE)");
					goto cleanup;
				}
			}

			/* wait for transfer done */
			do {
				if( UOS_SigWait( 1000, &sigCode )) {
					PrintUosError("UOS_SigWait");
					goto cleanup;
				}	
			} while( sigCode != UOS_SIG_USR1 );

			/* read entry from receive FIFO */
			if( (size = M_getblock( path, (u_int8*)rxFrmBuf, sizeof(rxFrmBuf) ))<0) {
				PrintError("M_getblock");
				goto cleanup;
			}

#if 0
			printf("tx:addr=0x%02x, val=0x%02x, rx: val=0x%02x)\n",
				txFrmBuf[ADDR], txFrmBuf[DATA], rxFrmBuf[DATA] );
#endif

			/* all regs? */
			if( singleRw == none ){
				rxBlkFrmBuf[i+2] = rxFrmBuf[DATA];	/* data */
			}
		} /* for */
	}/* Single Transfer */

	/* stop timer/QSPI */
	if( M_setstat(path, QSPIM_TIMER_STATE, FALSE )) {
		PrintError("M_setstat(QSPIM_TIMER_STATE)");
		goto cleanup;
	}

	/* all regs? */
	if( singleRw == none ){

		/* read */
		if( dump ){
			printf("=== read ===\n"); 
			RegDump( &rxBlkFrmBuf[2] );
		}
		/* write */
		else{
			printf("=== write ===\n"); 
			RegDump( G_cfgFrmBuf );
		}
	}
	/* one reg */
	else {
		/* read */
		if( singleRw == qspread ){
			printf("=== read from reg 0x%02x: 0x%02x ===\n", reg, rxFrmBuf[DATA] );
		}
		/* write */
		else {
			printf("=== write to reg 0x%02x: 0x%02x ===\n", reg, txFrmBuf[DATA] );
		}
	}

cleanup:
	/*--------------------+
    |  cleanup            |
    +--------------------*/
	M_setstat(path, QSPIM_FRM_SIG_CLR, 0 );

	if (M_close(path) < 0)
		PrintError("M_close");

	UOS_SigExit();
	return(0);
}

/********************************* RegDump **********************************
 *
 *  Description: Reg dump
 *			   
 *---------------------------------------------------------------------------
 *  Input......: ptr	ptr to data buffer
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void RegDump(u_int16 *ptr)
{
	int32 n, r, l=0;

	printf("0    1    2    3    4    5    6    7    8    9    a    b    c    d    e    f");
	
	for( n=0; n<KS_REG_NBR; n++ ){
	
		r = n%16;

		if( !r ){
			l++;
			printf("\n");

			switch( l ){
			case 1:
				printf("--- 0x00..0x0f --- global ---\n");
				break;
			case 2:
				printf("--- 0x10..0x1f --- port 1 ---\n");
				break;
			case 3:
				printf("--- 0x20..0x2f --- port 2 ---\n");
				break;
			case 4:
				printf("--- 0x30..0x3f --- port 3 ---\n");
				break;
			case 5:
				printf("--- 0x40..0x4f --- port 4 ---\n");
				break;
			case 6:
				printf("--- 0x50..0x5f --- port 5 ---\n");
				break;
			case 7:
				printf("--- 0x60..0x6d --- global ---\n");
				break;
			}
		}
		printf("0x%02x ",ptr[n]);
	}

	printf("\n");
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
	printf("*** %s failed: %s\n", info, M_errstring(UOS_ErrnoGet()));
}

/********************************* PrintUosError ****************************
 *
 *  Description: Print MDIS error message
 *			   
 *---------------------------------------------------------------------------
 *  Input......: info	info string
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void PrintUosError(char *info)
{
	printf("*** %s failed: %s\n", info, UOS_ErrString(UOS_ErrnoGet()));
}
