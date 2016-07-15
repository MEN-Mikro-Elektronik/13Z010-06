/****************************************************************************
 ************                                                    ************
 ************                   QSPIM_FRAM                       ************
 ************                                                    ************
 ****************************************************************************
 *
 *       Author: michael.roth@men.de
 *        $Date: 2014/07/22 10:27:27 $
 *    $Revision: 1.2 $
 *
 *  Description: Test tool for a QSPI connected FRAM
 *               (RAMTRON FM25H20 256k x 8 - 5280-0039)
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *     Switches: -
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: qspim_fram.c,v $
 * Revision 1.2  2014/07/22 10:27:27  ts
 * R: warning at 64bit linux, gcc 4.8: cast to integer of different size
 * M: cast M_SG_BLOCK address of blk to INT32_OR_64, not int32
 *
 * Revision 1.1  2014/03/28 13:54:31  MRoth
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2014 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/
static const char RCSid[]="$Id: qspim_fram.c,v 1.2 2014/07/22 10:27:27 ts Exp $";

/*-------------------------------------+
|    INCLUDES                          |
+-------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/qspim_drv.h>

/*-------------------------------------+
|    DEFINES                           |
+-------------------------------------*/
/* RAMTRON FM25H20 256k x 8 5280-0039 */
#define FM_BAUDRATE     10000000    /* max. 10MHz */
#define FM_WORD_SIZE    8           /* 8 bits per word */
#define FM_CPOL         0           /* base value of the clock is zero */
#define FM_CPHA         0           /* data captured on rising edge, data propagated on falling edge */
#define FM_DSCLK        50          /* tSH >10ns */
#define FM_DTL          100         /* tD >40ns */
#define FM_REG_NBR      0x3FFFF     /* register 0x00000..0x3FFFF (256k)*/
#define MAX_DATA_WORDS  (0xFF-4)    /* cmd(1), addr(3), regs(0xfb) */
#define MAX_WORDS       0xFF        /* max. value for block transfers */

/* FRAM opcodes */
#define CMD_WREN        0x06    /* 0110 - Write Enable*/
#define CMD_RDSR        0x05    /* 0101 - Read status register */
#define CMD_WRSR        0x01    /* 0001 - Write status register */
#define CMD_WRITE       0x02
#define CMD_READ        0x03

/* FRAM frame structure (five 8bit words) */
#define CMD             0
#define ADDR_MSB        1
#define ADDR            2
#define ADDR_LSB        3
#define DATA            4

/*-------------------------------------+
|    TYPEDEFS                          |
+-------------------------------------*/
typedef enum {
	none,
	qspread,
	qspwrite
} RW_TYPE;

/*-------------------------------------+
|    PROTOTYPES                        |
+-------------------------------------*/
static void usage( void );
static void PrintError( char *info );
static void PrintUosError( char *info );

/************************************ usage *********************************/
/** Prints the program usage
 */
static void usage( void )
{
	printf("\nUsage:    qspim_fram <device> [<opts>]");
	printf("\nFunction: QSPIM test tool to access the RAMTRON FM25H20 FRAM");
	printf("\nOptions:\n");
	printf("    device              device name                          [none]\n");
	printf("    -r=<reg>            read from register <reg>             [0x0] \n");
	printf("    -r=<reg> -b=<byte>  write <byte> to register <reg>       [0x0] \n");
	printf("    [-n=<num>]          number of bytes to read/write        [1] \n");
	printf("    -m                  simple memory test using block transfers \n");
	printf("\n");
	printf("Notes:\n");
	printf("- <reg> = 0x0..0x%x\n", FM_REG_NBR);
	printf("\n");
	printf("Calling examples:\n");
	printf("\n1) read one byte from offset 0x0: \n");
	printf("     qspim_fram z76_qspim_1 \n");
	printf("\n2) read 10 bytes from offset 0x12: \n");
	printf("     qspim_fram z76_qspim_1 -r=0x12 -n=10 \n");
	printf("\n3) write value 0xab to offset 0x01:\n");
	printf("     qspim_fram z76_qspim_1 -r=0x01 -b=0xab \n");
	printf("\n4) simple memory test: \n");
	printf("     qspim_fram z76_qspim_1 -m \n");
	printf("\n(c)Copyright 2014 by MEN Mikro Elektronik GmbH\n%s\n\n", RCSid);
}

/****************************************************************************/
/** Program main function
 *
 *  \param argc    \IN argument counter
 *  \param argv    \IN argument vector
 *
 *  \return        success (0) or error (1)
 */
int main( int argc, char *argv[] )
{
	char          *deviceP=NULL, *str=NULL, *errstr, buf[40];
	int           i=0, memtest;
	u_int32       sigCode, reg=0, num=1, n=0, k=0, val=0, errcount=0;
	u_int16       defFrm;
	u_int16       txFrmBuf[5]={0x0}, rxFrmBuf[5]={0x0};
	u_int16       txBlkFrmBuf[MAX_WORDS]={0x0}, rxBlkFrmBuf[MAX_WORDS]={0x0};
	u_int16       bytepat[MAX_DATA_WORDS]={0};
	RW_TYPE       readwrite=none;
	M_SG_BLOCK    blk;
	MDIS_PATH     path;

	/*--------------------+
	|  check arguments    |
	+--------------------*/
	if( (errstr = UTL_ILLIOPT("r=b=n=m?", buf)) ) {	/* check args */
		printf("*** %s\n", errstr);
		usage();
		return(1);
	}

	if( UTL_TSTOPT("?") ) {							/* help requested ? */
		usage();
		return(1);
	}

	/*--------------------+
	|  get arguments      |
	+--------------------*/
	for( i=1; i<argc; i++ ) {
		if( *argv[i] != '-' ) {
			if( deviceP == NULL ) {
				deviceP = argv[i];
				break;
			}
		}
	}

	if( !deviceP ) {
		printf( "\n***ERROR: missing QSPIM device name!\n" );
		usage();
		return(1);
	}

	/* register/offset to write/read */
	if( (str = UTL_TSTOPT("r=")) ) {
		sscanf( str, "%x", &reg );
		if( reg > FM_REG_NBR ) {
			printf("*** -r=0x%x out of range\n", reg);
			usage();
			return(1);
		}
		readwrite=qspread;
	}
	else {
		reg = 0x00;
	}

	/* register/offset to write/read */
	if( (str = UTL_TSTOPT("n=")) ) {
		sscanf( str, "%x", &num );
		if( (reg+num-1) > FM_REG_NBR ) {
			printf("*** reg+num > 0x%x --> out of range\n", FM_REG_NBR);
			usage();
			return(1);
		}
	}
	else {
		num = 1;
	}

	/* write access */
	if( (str = UTL_TSTOPT("b=")) ) {
		sscanf( str, "%x", &val );
		readwrite=qspwrite;
	}

	memtest = ( UTL_TSTOPT("m") ? 1 : 0 );

	if( memtest && (readwrite != none) ) {
		printf("*** -m cannot be used together with -r/-b \n");
		usage();
		return(1);
	}

	/*--------------------+
	|  open path          |
	+--------------------*/
	if( (path = M_open(deviceP)) < 0 ) {
		PrintError("M_open");
		return(1);
	}

	/* init signal handling */
	if( UOS_SigInit(NULL) ) {
		PrintUosError("UOS_SigInit");
		goto cleanup;
	}

	/* install signal 1 */
	if( UOS_SigInstall(UOS_SIG_USR1) ) {
		PrintUosError("UOS_SigInstall(UOS_SIG_USR1)");
		goto cleanup;
	}

	/*--------------------+
	|  config             |
	+--------------------*/
	/* prevent frame duplication */
	if( M_setstat( path, QSPIM_NO_DUPLICATION, 1 ) ) {
		PrintError("M_setstat(QSPIM_NO_DUPLICATION)");
		goto cleanup;
	}

	/* set word size */
	if( M_setstat( path, QSPIM_BITS, FM_WORD_SIZE ) ) {
		PrintError("M_setstat(QSPIM_BITS)");
		goto cleanup;
	}

	/* set cycle timer (to slowest possible value) */
	if( M_setstat( path, QSPIM_TIMER_CYCLE_TIME, 1600000 ) ) {
		PrintError("M_setstat(QSPIM_TIMER_CYCLE_TIME)");
		goto cleanup;
	}

	/* set baudrate */
	if( M_setstat( path, QSPIM_BAUD, FM_BAUDRATE ) ) {
		PrintError("M_setstat(QSPIM_BAUD)");
		goto cleanup;
	}

	/* set clock polarity */
	if( M_setstat( path, QSPIM_CPOL, FM_CPOL ) ) {
		PrintError("M_setstat(QSPIM_CPOL)");
		goto cleanup;
	}

	/* set clock phase  */
	if( M_setstat( path, QSPIM_CPHA, FM_CPHA ) ) {
		PrintError("M_setstat(QSPIM_CPHA)");
		goto cleanup;
	}

	/* set default state of PCS3..0 */
	if( M_setstat( path, QSPIM_PCS_DEFSTATE, 0xf ) ) {
		PrintError("M_setstat(QSPIM_PCS_DEFSTATE)");
		goto cleanup;
	}

	/* set PCS to SCLK delay */
	if( M_setstat( path, QSPIM_DSCLK, FM_DSCLK ) ) {
		PrintError("M_setstat(QSPIM_DSCLK)");
		goto cleanup;
	}

	/* set delay after transfer */
	if( M_setstat( path, QSPIM_DTL, FM_DTL ) ) {
		PrintError("M_setstat(QSPIM_DTL)");
		goto cleanup;
	}

	/* install signal to be sent when QSPI frame transferred */
	if( M_setstat( path, QSPIM_FRM_SIG_SET, UOS_SIG_USR1 ) ) {
		PrintError("M_setstat(QSPIM_FRM_SIG_SET)");
		goto cleanup;
	}

	/***********************************************************************
	 * Configure QSPI frame
	 *
	 * Bit 15    14    13    12    11    10    9     8     7 ... 0
	 * -----------------------------------------------------------
	 *     CONT  BITSE DT    DSCLK PCS3  PCS2  PCS1  PCS0  NWORDS
	 *
	 * Activate BITSE, DT, DSCLK for all slaves, disable CONT      (0x7...)
	 * Use /CS0                                                    (0x.e..)
	 * Four slaves /CSx example: /CS0    (0x.e..)
	 *                           /CS1    (0x.d..)
	 *                           /CS2    (0x.b..)
	 *                           /CS3    (0x.7..)
	 * Use transfer with:
	 *        five  words (cmd(1) + addr(3) + data(1))             (0x..05)
	 *        MAX_WORDS   (cmd(1) + addr(3) + data(0xfb))          (0x..ff)
	 **********************************************************************/
	defFrm  = 0x7e00;
	if( memtest )
		defFrm |= MAX_WORDS;
	else
		defFrm |= 0x05;

	blk.data = (void*)&defFrm;
	blk.size = sizeof(defFrm);

	if( M_setstat( path, QSPIM_BLK_DEFINE_FRM, (INT32_OR_64)&blk ) ) {
		PrintError("M_setstat(QSPIM_BLK_DEFINE_FRM)");
		goto cleanup;
	}

	if( !memtest ) {
		/*------------------------+
		|  Single Transfer        |
		+------------------------*/

		if( readwrite == qspwrite ) {
			/* Send write enable */
			txFrmBuf[CMD]      = CMD_WREN;		/* command */
			txFrmBuf[ADDR_MSB] = 0x00;			/* address (unused) */
			txFrmBuf[ADDR]     = 0x00;			/* address (unused) */
			txFrmBuf[ADDR_LSB] = 0x00;			/* address (unused) */
			txFrmBuf[DATA]     = 0x00;			/* data (unused) */

			if( M_setblock( path, (u_int8*)txFrmBuf, sizeof(txFrmBuf) ) < 0 ) {
				PrintError("M_setblock");
				goto cleanup;
			}
			/* start timer/QSPI */
			if( M_setstat( path, QSPIM_TIMER_STATE, TRUE ) ) {
				PrintError("M_setstat(QSPIM_TIMER_STATE)");
				goto cleanup;
			}
			/* wait for transfer done */
			do {
				if( UOS_SigWait(1000, &sigCode) ) {
					PrintUosError("UOS_SigWait");
					goto cleanup;
				}
			}while( sigCode != UOS_SIG_USR1 );

			/* set status register bits to 0 (disable blockprotect) */
			txFrmBuf[CMD]      = CMD_WRSR;		/* command */
			txFrmBuf[ADDR_MSB] = 0x00;			/* address (unused) */

			if( M_setblock( path, (u_int8*)txFrmBuf, sizeof(txFrmBuf) ) < 0 ) {
				PrintError("M_setblock");
				goto cleanup;
			}
			/* wait for transfer done */
			do {
				if( UOS_SigWait( 1000, &sigCode )) {
					PrintUosError("UOS_SigWait");
					goto cleanup;
				}
			} while( sigCode != UOS_SIG_USR1 );

			printf("Write --> ");

			for( n=reg; n<(reg+num); n++ ) {

				/* Send write enable */
				txFrmBuf[CMD]      = CMD_WREN;		/* command */
				txFrmBuf[ADDR_MSB] = 0x00;			/* address (unused) */
				txFrmBuf[ADDR]     = 0x00;			/* address (unused) */
				txFrmBuf[ADDR_LSB] = 0x00;			/* address (unused) */
				txFrmBuf[DATA]     = 0x00;			/* data (unused) */

				if( M_setblock( path, (u_int8*)txFrmBuf, sizeof(txFrmBuf) ) < 0 ) {
					PrintError("M_setblock");
					goto cleanup;
				}
				/* wait for transfer done */
				do {
					if( UOS_SigWait(1000, &sigCode) ) {
						PrintUosError("UOS_SigWait");
						goto cleanup;
					}
				}while( sigCode != UOS_SIG_USR1 );

				/* Write frame */
				txFrmBuf[CMD]      = CMD_WRITE;						/* command */
				txFrmBuf[ADDR_MSB] = (u_int16)((n&0x00ff0000)>>16);
				txFrmBuf[ADDR]     = (u_int16)((n&0x0000ff00)>>8);
				txFrmBuf[ADDR_LSB] = (u_int16) (n&0x000000ff);
				txFrmBuf[DATA]     = (u_int16) (val&0x000000ff);	/* data */

				/* send frame data */
				if( M_setblock( path, (u_int8*)txFrmBuf, sizeof(txFrmBuf) ) < 0 ) {
					PrintError("M_setblock");
					goto cleanup;
				}
				/* wait for transfer done */
				do {
					if( UOS_SigWait(1000, &sigCode) ) {
						PrintUosError("UOS_SigWait");
						goto cleanup;
					}
				}while( sigCode != UOS_SIG_USR1 );
			}
			printf("OK\n");
		}
		else { /* read */
			for( n=reg; n<(reg+num); n++ ) {

				printf("Read --> ");

				/* Read frame */
				txFrmBuf[CMD]      = (u_int16)CMD_READ;		/* command */
				txFrmBuf[ADDR_MSB] = (u_int16)((n&0x00ff0000)>>16);
				txFrmBuf[ADDR]     = (u_int16)((n&0x0000ff00)>>8);
				txFrmBuf[ADDR_LSB] = (u_int16) (n&0x000000ff);
				txFrmBuf[DATA]     = 0x00;					/* data (unused) */

				/* send frame data */
				if( M_setblock( path, (u_int8*)txFrmBuf, sizeof(txFrmBuf) ) < 0 ) {
					PrintError("M_setblock");
					goto cleanup;
				}
				/* start timer/QSPI */
				if( M_setstat( path, QSPIM_TIMER_STATE, TRUE ) ) {
					PrintError("M_setstat(QSPIM_TIMER_STATE)");
					goto cleanup;
				}
				/* wait for transfer done */
				do {
					if( UOS_SigWait(1000, &sigCode) ) {
						PrintUosError("UOS_SigWait");
						goto cleanup;
					}
				}while( sigCode != UOS_SIG_USR1 );

				/* read entry from receive FIFO */
				if( M_getblock( path, (u_int8*)rxFrmBuf, sizeof(rxFrmBuf) ) < 0 ) {
					PrintError("M_getblock");
					goto cleanup;
				}
				printf("reg: 0x%05x, val=0x%02x\n", n, rxFrmBuf[DATA]);
			}
		}
	}
	else { /* memtest */
		/*------------------------+
		|  Block Transfer         |
		+------------------------*/

		printf("\nSimple memory test\n");

		/* Send write enable */
		txBlkFrmBuf[CMD]      = CMD_WREN;		/* command */
		txBlkFrmBuf[ADDR_MSB] = 0x00;			/* address (unused) */
		txBlkFrmBuf[ADDR]     = 0x00;			/* address (unused) */
		txBlkFrmBuf[ADDR_LSB] = 0x00;			/* address (unused) */
		/* data (unused) */
		memset( (void*)&txBlkFrmBuf[DATA], 0x00, sizeof(bytepat) ); 

		if( M_setblock( path, (u_int8*)txBlkFrmBuf, sizeof(txBlkFrmBuf) ) < 0 ) {
			PrintError("M_setblock");
			goto cleanup;
		}
		/* start timer/QSPI */
		if( M_setstat( path, QSPIM_TIMER_STATE, TRUE ) ) {
			PrintError("M_setstat(QSPIM_TIMER_STATE)");
			goto cleanup;
		}
		/* wait for transfer done */
		do {
			if( UOS_SigWait(1000, &sigCode) ) {
				PrintUosError("UOS_SigWait");
				goto cleanup;
			}
		}while( sigCode != UOS_SIG_USR1 );

		/* set status register bits to 0 */
		txBlkFrmBuf[CMD]      = CMD_WRSR;		/* command */
		txBlkFrmBuf[ADDR_MSB] = 0x00;			/* address (unused) */

		if( M_setblock( path, (u_int8*)txBlkFrmBuf, sizeof(txBlkFrmBuf) ) < 0 ) {
			PrintError("M_setblock");
			goto cleanup;
		}
		/* wait for transfer done */
		do {
			if( UOS_SigWait(1000, &sigCode) ) {
				PrintUosError("UOS_SigWait");
				goto cleanup;
			}
		}while( sigCode != UOS_SIG_USR1 );

		/* stop timer/QSPI */
		if( M_setstat( path, QSPIM_TIMER_STATE, FALSE ) ) {
			PrintError("M_setstat(QSPIM_TIMER_STATE)");
			goto cleanup;
		}

		/* Generate simple ascending test pattern */
		for( i=0; i<MAX_DATA_WORDS; i++ ) {
			bytepat[i] = (u_int16)i+1;
		}

		printf("\nWrite       --> ");

		for( reg=0; reg<(FM_REG_NBR-MAX_DATA_WORDS); reg=reg+MAX_DATA_WORDS ) {
			/* Send write enable */
			txBlkFrmBuf[CMD]      = CMD_WREN;		/* command */
			txBlkFrmBuf[ADDR_MSB] = 0x00;			/* address (unused) */
			txBlkFrmBuf[ADDR]     = 0x00;			/* address (unused) */
			txBlkFrmBuf[ADDR_LSB] = 0x00;			/* address (unused) */
			/* data (unused) */
			memset( (void*)&txBlkFrmBuf[DATA], 0x00, sizeof(bytepat) ); 

			if( M_setblock( path, (u_int8*)txBlkFrmBuf, sizeof(txBlkFrmBuf) ) < 0 ) {
				PrintError("M_setblock");
				goto cleanup;
			}
			/* start timer/QSPI */
			if( M_setstat( path, QSPIM_TIMER_STATE, TRUE ) ) {
				PrintError("M_setstat(QSPIM_TIMER_STATE)");
				goto cleanup;
			}
			/* wait for transfer done */
			do {
				if( UOS_SigWait(1000, &sigCode) ) {
					PrintUosError("UOS_SigWait");
					goto cleanup;
				}
			}while( sigCode != UOS_SIG_USR1 );

			/* Write frame */
			txBlkFrmBuf[CMD]      = CMD_WRITE;						/* command */
			txBlkFrmBuf[ADDR_MSB] = (u_int16)((reg&0x00ff0000)>>16);
			txBlkFrmBuf[ADDR]     = (u_int16)((reg&0x0000ff00)>>8);
			txBlkFrmBuf[ADDR_LSB] = (u_int16) (reg&0x000000ff);

			memcpy( (void*)&txBlkFrmBuf[DATA], (void*)bytepat, sizeof(bytepat) );

			/* send frame data */
			if( M_setblock( path, (u_int8*)txBlkFrmBuf, sizeof(txBlkFrmBuf) ) < 0 ) {
				PrintError("M_setblock");
				goto cleanup;
			}
			/* wait for transfer done */
			do {
				if( UOS_SigWait(1000, &sigCode) ) {
					PrintUosError("UOS_SigWait");
					goto cleanup;
				}
			}while( sigCode != UOS_SIG_USR1 );

			/* stop timer/QSPI */
			if( M_setstat( path, QSPIM_TIMER_STATE, FALSE ) ) {
				PrintError("M_setstat(QSPIM_TIMER_STATE)");
				goto cleanup;
			}
			k++;
		}
		printf("OK (%d block write operations)\n", k);
		k=0;

		printf("Read/Verify --> ");

		for( reg=0; reg<(FM_REG_NBR-MAX_DATA_WORDS); reg=reg+MAX_DATA_WORDS ) {
			/* Read frame */
			txBlkFrmBuf[CMD]      = (u_int16)CMD_READ;	/* command */
			txBlkFrmBuf[ADDR_MSB] = (u_int16)((reg&0x00ff0000)>>16);
			txBlkFrmBuf[ADDR]     = (u_int16)((reg&0x0000ff00)>>8);
			txBlkFrmBuf[ADDR_LSB] = (u_int16) (reg&0x000000ff);

			/* data (unused) */
			memset( (void*)&txBlkFrmBuf[DATA], 0x00, sizeof(bytepat) ); 

			/* send frame data */
			if( M_setblock( path, (u_int8*)txBlkFrmBuf, sizeof(txBlkFrmBuf) ) < 0 ) {
				PrintError("M_setblock");
				goto cleanup;
			}
			/* start timer/QSPI */
			if( M_setstat( path, QSPIM_TIMER_STATE, TRUE ) ) {
				PrintError("M_setstat(QSPIM_TIMER_STATE)");
				goto cleanup;
			}
			/* wait for transfer done */
			do {
				if( UOS_SigWait(1000, &sigCode) ) {
					PrintUosError("UOS_SigWait");
					goto cleanup;
				}	
			}while( sigCode != UOS_SIG_USR1 );

			/* read entry from receive FIFO */
			if( M_getblock( path, (u_int8*)rxBlkFrmBuf, sizeof(rxBlkFrmBuf) ) < 0 ) {
				PrintError("M_getblock");
				goto cleanup;
			}
			/* verify */
			if( memcmp( bytepat, rxBlkFrmBuf+4, sizeof(bytepat) ) ) {
				errcount++;
			}

			/* stop timer/QSPI */
			if( M_setstat( path, QSPIM_TIMER_STATE, FALSE ) ) {
				PrintError("M_setstat(QSPIM_TIMER_STATE)");
				goto cleanup;
			}
			k++;
		}
		if( errcount ) {
			printf("FAIL %d block read operations)\n", k);
		}
		else {
			printf("OK (%d block read operations)\n", k);
		}

		printf("\n------------------------------------------------\n");
		printf("TEST RESULT: %d errors --> ", errcount);
		if( errcount ) {
			printf("FAIL\n");
		}
		else {
			printf("PASS\n");
		}
	} /* memory test */

cleanup:
	/*--------------------+
	|  cleanup            |
	+--------------------*/
	M_setstat( path, QSPIM_FRM_SIG_CLR, 0 );

	if( M_close(path) < 0 )
		PrintError("M_close");

	UOS_SigExit();

	return(0);
}


/********************************* PrintError *******************************/
/** Routine to print MDIS error messages
 *
 *  \param info       \IN info string
 *
 */
static void PrintError( char *info )
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}


/******************************** PrintUosError *****************************/
/** Routine to print signal related error messages
 *
 *  \param info       \IN info string
 *
 */
static void PrintUosError( char *info )
{
	printf("*** %s failed: %s\n", info, UOS_ErrString(UOS_ErrnoGet()));
}
