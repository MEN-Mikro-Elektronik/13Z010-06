/****************************************************************************
 * Copyright 2019, MEN Mikro Elektronik GmbH
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
#include <stdlib.h>

#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/z17_drv.h>
#include <MEN/qspim_drv.h>

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

#define BIT(x) (1 << (x))
const int port = 31;

#define FM_BAUDRATE 1000000	/* max. 5 MHz */
#define FM_WORD_SIZE 8			/* 8 bits per word */
#define FM_CPOL 0				/* base value of the clock is 0 */
#define FM_CPHA 0				/* data captured on rising edge,
								   data propagated on falling edge */
#define FM_DSCLK 150 			/* tCSS > 80 ns */
#define FM_DTL 100				/* tCSH > 80 ns */

/* EEPROM opcodes */
#define CMD_WRSR  0x01			/*  Write status register */
#define CMD_WRITE 0x02
#define CMD_READ  0x03
#define CMD_WREN  0x06			/* Write Enable */
#define CMD_RDSR  0x05			/*  Read status register */

/* EEPROM frame structure */
#define CMD 0
#define ADDR_MSB 1
#define ADDR_LSB 2
#define DATA 3

/* #define DEBUG 1 */

enum chip_select {
	CS0 = 0,
	CS1 = 1,
	CS2 = 2,
	CS3 = 3
};

#ifdef DEBUG
# define printd(fmt, ...)						\
	do {										\
		printf(fmt, __VA_ARGS__);				\
	} while(0)

static void hexdump(unsigned char *d, int bytes)
{
		int i;
		unsigned char *data = (unsigned char *) d;

		for (i = 0; i < bytes; i++) {
				if (i % 8 == 0) {
						if (i > 0)
								printf("\n");
						printf("%08x: ", i);
				}

				printf("0x%02x ", (unsigned int) data[i]);
		}
		printf("\n");
}

#else
# define printd(fmt, ...)
static void hexdump(unsigned char *d, int bytes)
{
}
#endif

static void PrintError(const char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}

static void PrintUosError(const char *info)
{
	printf("*** %s failed: %s\n", info, UOS_ErrString(UOS_ErrnoGet()));
}

static void usage(const char *pname)
{
	printf("Usage: %s gpio-device qspi-device\n", pname);
}

/********************************************
 * Set GPIO 31 high to activate QSPI output *
 *******************************************/
static MDIS_PATH ad48n_set_pinmux(const char *gpio_device)
{
	int ret;
	int32 dir;
	MDIS_PATH gpio_path;

	if (!gpio_device) {
		return(-1);
	}

	gpio_path = M_open(gpio_device);
	if (gpio_path < 0) {
		PrintError("open");
		return(1);
	}

	ret = M_getstat(gpio_path, Z17_DIRECTION, &dir);
	if (ret < 0) {
		PrintError("getstat Z17_DIRECTION");
		goto abort;
	}

	dir |= BIT(port);

	ret = M_setstat(gpio_path, Z17_DIRECTION, dir);
	if (ret < 0) {
		PrintError("setstat Z17_DIRECTION");
		goto abort;
	}

	ret = M_setstat(gpio_path, Z17_SET_PORTS, BIT(port));
	if (ret < 0) {
		PrintError("setstat Z17_SET_PORTS");
		goto abort;
	}

	return gpio_path;

abort:
	M_close(gpio_path);
	return(-1);
}

static MDIS_PATH ad48n_setup_qspim(const char *qspi_device)
{
	MDIS_PATH qspim_path;
	int ret;

	qspim_path = M_open(qspi_device);
	if (qspim_path < 0) {
		PrintError("open QSPI");
		return(-1);
	}

	ret = UOS_SigInit(NULL);
	if (ret) {
		PrintUosError("UOS_SigInit");
		goto abort;
	}

	ret = UOS_SigInstall(UOS_SIG_USR1);
	if (ret) {
		PrintUosError("UOS_SigInstall(UOS_SIG_USR1");
		goto abort;
	}

	/* prevent frame duplication */
	ret = M_setstat(qspim_path, QSPIM_NO_DUPLICATION, 1);
	if (ret) {
		PrintError("setstat QSPIM_NO_DUPLICATION");
		goto abort;
	}

	/* set word size */
	ret = M_setstat(qspim_path, QSPIM_BITS, FM_WORD_SIZE);
	if (ret) {
		PrintError("setstat QSPIM_BITS");
		goto abort;
	}

	/* set cycle timer (to slowest possible value) */
	ret = M_setstat(qspim_path, QSPIM_TIMER_CYCLE_TIME, 160000);
	if (ret) {
		PrintError("setstat QSPIM_TIMER_CYCLE_TIME");
		goto abort;
	}

	/* set baudrate */
	ret = M_setstat(qspim_path, QSPIM_BAUD, FM_BAUDRATE);
	if (ret) {
		PrintError("setstat QSPIM_BAUD");
		goto abort;
	}

	/* set clock polarity */
	ret = M_setstat(qspim_path, QSPIM_CPOL, FM_CPOL);
	if (ret) {
		PrintError("setstat QSPIM_CPOL");
		goto abort;
	}

	/* set clock phase */
	ret = M_setstat(qspim_path, QSPIM_CPHA, FM_CPHA);
	if (ret) {
		PrintError("setstat QSPIM_CPHA");
		goto abort;
	}

	/* set default state of PCS[3..0] */
	ret = M_setstat(qspim_path, QSPIM_PCS_DEFSTATE, 0xf);
	if (ret) {
		PrintError("setstat QSPIM_PCS_DEFSTATE");
		goto abort;
	}

	/* set PCS to SCLK delay */
	ret = M_setstat(qspim_path, QSPIM_DSCLK, FM_DSCLK);
	if (ret) {
		PrintError("setstat QSPIM_DSCLK");
		goto abort;
	}

	/* set delay after transfer */
	ret = M_setstat(qspim_path, QSPIM_DTL, FM_DTL);
	if (ret) {
		PrintError("setstat QSPIM_DTL");
		goto abort;
	}

	/* install signal to be sent when QSPI frame transferred */
	ret = M_setstat(qspim_path, QSPIM_FRM_SIG_SET, UOS_SIG_USR1);
	if (ret) {
		PrintError("setstat QSPIM_FRM_SIG_SET");
		goto abort;
	}

	return(qspim_path);

abort:
	M_close(qspim_path);
	return(-1);
}

/************************************************************************
 * Configure QSPI frame                                                 *
 *                                                                      *
 * |------+-------+----+-------+------+------+------+------+--------|   *
 * |   15 |    14 | 13 |    12 |   11 |   10 |    9 |    8 |   7..0 |   *
 * |------+-------+----+-------+------+------+------+------+--------|   *
 * | CONT | BITSE | DT | DSCLK | PCS3 | PCS2 | PCS1 | PCS0 | NWORDS |   *
 * |------+-------+----+-------+------+------+------+------+--------|   *
 *                                                                      *
 * Activate BITSE, DT, DSCLK for all salves, disable CONT.              *
 * Set NWORDS to 255.                                                   *
 ***********************************************************************/
static int ad48n_qspi_config_frame(MDIS_PATH qspim_path, enum chip_select pcs,
				   u_int8 len)
{
	M_SG_BLOCK blk;
	u_int16 frame = 0;
	u_int8 cs = 0;
	int ret;

	/* Activate BITSE, DT, DSCLK for all slaves, disable CONT */
	/* frame |= 0x7 << 12; */
	frame |= 0x5 << 12;

	switch (pcs) {
	case CS0:
		cs = 0x0e;
		break;
	case CS1:
		cs = 0x0d;
		break;
	case CS2:
		cs = 0x0b;
		break;
	case CS3:
		cs = 0x07;
		break;
	}

	frame |= cs << 8;
	frame |= len;

	blk.data = (void *)&frame;
	blk.size = sizeof(frame);

	ret = M_setstat(qspim_path, QSPIM_BLK_DEFINE_FRM, (INT32_OR_64)&blk);
	if (ret) {
		PrintError("setstat QSPIM_BLK_DEFINE_FRM");
		return(-1);
	}

	return(0);
}

static int ad48n_qspi_tx_blk(MDIS_PATH qspim_path, u_int16 *txblk,
							 u_int32 txblk_size)
{
	int ret;
	u_int32 sig;

	hexdump((unsigned char *) txblk, txblk_size);

	/* send frame data */
	ret = M_setblock(qspim_path, (u_int8 *)txblk, txblk_size);
	if (ret < 0) {
		PrintError("setblock");
		goto abort;
	}

	/* start timer/QSPI */
	ret = M_setstat(qspim_path, QSPIM_TIMER_STATE, TRUE);
	if (ret) {
		PrintError("setstat QSPIM_TIMER_STATE");
		goto abort;
	}

	do {
		ret = UOS_SigWait(1000, &sig);
		if (ret) {
			PrintUosError("UOS_SigWait");
			goto abort;
		}
	} while (sig != UOS_SIG_USR1);

	/* stop timer/QSPI */
	ret = M_setstat(qspim_path, QSPIM_TIMER_STATE, FALSE);
	if (ret) {
		PrintError("setstat QSPIM_TIMER_STATE");
		goto abort;
	}

	return(0);

abort:
	return(-1);
}

static int ad48n_qspi_send_cmd(MDIS_PATH qspim_path, enum chip_select pcs,
							   u_int8 cmd, u_int16 reg)
{
	u_int16 txblk[3] = { 0 };
	int ret;

	txblk[CMD] = cmd;
	txblk[ADDR_MSB] = (u_int8) ((reg & 0xff00) >> 8);
	txblk[ADDR_LSB] = (u_int8) (reg & 0x00ff);

	ret = ad48n_qspi_config_frame(qspim_path, pcs, ARRAY_SIZE(txblk));
	if (ret < 0) {
		goto abort;
	}

	ret = ad48n_qspi_tx_blk(qspim_path, txblk, sizeof(txblk));
	if (ret < 0) {
		goto abort;
	}

	return(0);

abort:
	return(-1);
}

static int ad48n_qspi_send_data(MDIS_PATH qspim_path, enum chip_select pcs,
								u_int16 reg, char din)
{
	int ret;
	int len = 4;
	u_int16 txblk[len];

	ret = ad48n_qspi_config_frame(qspim_path, pcs, ARRAY_SIZE(txblk));
	if (ret < 0) {
		goto abort;
	}

	txblk[CMD] = CMD_WRITE;
	txblk[ADDR_MSB] = (u_int8) ((reg & 0xff00) >> 8);
	txblk[ADDR_LSB] = (u_int8) (reg & 0x00ff);
	txblk[DATA] = din;

	hexdump((unsigned char *) txblk, len);

	ret = ad48n_qspi_tx_blk(qspim_path, txblk, sizeof(txblk));
	if (ret < 0) {
		goto abort;
	}

	printd("%d block write operations\n", len);

	return(0);

abort:
	return(-1);
}

static int ad48n_qspi_write(MDIS_PATH qspim_path, enum chip_select pcs,
							u_int16 reg, char din)
{
	int ret;

	printd("Sending CMD_WREN on pcs: %d\n", pcs);
	ret = ad48n_qspi_send_cmd(qspim_path, pcs, CMD_WREN, reg);
	if (ret < 0) {
		goto abort;
	}

	printd("Sending data on pcs: %d\n", pcs);
	ret = ad48n_qspi_send_data(qspim_path, pcs, reg, din);
	if (ret < 0) {
		goto abort;
	}

	return 0;

abort:
	return(-1);
}

static char ad48n_qspi_read(MDIS_PATH qspim_path, enum chip_select pcs,
							u_int16 reg)
{
	int ret;
	u_int16 rxblk[4] = { 0 };
	u_int32 sig;

	rxblk[CMD] = CMD_READ;
	rxblk[ADDR_MSB] = (u_int8) ((reg & 0xff00) >> 8);
	rxblk[ADDR_LSB] = (u_int8) (reg & 0x00ff);
	rxblk[DATA] = 0;

	ret = ad48n_qspi_config_frame(qspim_path, pcs, sizeof(rxblk)/2);
	if (ret < 0) {
		goto abort;
	}

	/* send frame data */
	ret = M_setblock(qspim_path, (u_int8 *)rxblk, sizeof(rxblk));
	if (ret < 0) {
		PrintError("setblock");
		goto abort;
	}

	/* start timer/QSPI */
	ret = M_setstat(qspim_path, QSPIM_TIMER_STATE, TRUE);
	if (ret) {
		PrintError("setstat QSPIM_TIMER_STATE");
		goto abort;
	}

	do {
		ret = UOS_SigWait(1000, &sig);
		if (ret) {
			PrintUosError("UOS_SigWait");
			goto abort;
		}
	} while (sig != UOS_SIG_USR1);

	/* stop timer/QSPI */
	ret = M_setstat(qspim_path, QSPIM_TIMER_STATE, FALSE);
	if (ret) {
		PrintError("setstat QSPIM_TIMER_STATE");
		goto abort;
	}


	ret = M_getblock(qspim_path, (u_int8*)rxblk, sizeof(rxblk));
	if (ret < 0) {
		PrintError("M_getblock");
		goto abort;
	}

	hexdump((unsigned char *)rxblk, sizeof(rxblk));
	printd("%d block read operations\n", sizeof(rxblk));

	return rxblk[DATA];

abort:
	return -1;
}


/*************************************************************************/
/** Program main function
 *
 * \param argc	\IN argument counter
 * \param argv	\IN argument vector
 *
 * \return 		success (o) or error (1)
 */
int main(int argc, char **argv)
{
	int32 n;
	char *gpio_device = NULL;
	char *qspim_device = NULL;
	char *errstr;
	char buf[40];
	MDIS_PATH gpio_path;
	MDIS_PATH qspim_path;
	int write = 0;
	enum chip_select pcs;

	errstr = UTL_ILLIOPT("w?", buf);
	if (errstr) {
		printf("*** %s\n", errstr);
		return(1);
	}

	if (UTL_TSTOPT("?")) {
		usage(argv[0]);
		return(1);
	}

	if (UTL_TSTOPT("w")) {
		write = 1;
	}

	if (argc < 3) {
		usage(argv[0]);
		return(1);
	}

	/*****************
	 * get arguments *
	 *****************/
	for (n = 1; n < argc; n++) {
		if (*argv[n] == '-') {
			continue;
		}
		gpio_device = argv[n];
		break;
	}

	for (n = 1; n < argc; n++) {
		if (*argv[n] == '-') {
			continue;
		}
		if (!strcmp(argv[n], gpio_device)) {
			continue;
		}
		qspim_device = argv[n];
		break;
	}

	if (!qspim_device) {
		usage(argv[0]);
		return(1);
	}

	if (!gpio_device) {
		usage(argv[0]);
		return(1);
	}

	printd("Using GPIO Device: %s\n", gpio_device);
	printd("Using QSPI Device: %s\n", qspim_device);

	gpio_path = ad48n_set_pinmux(gpio_device);
	if (gpio_path < 0) {
		printf("*** can't set-up pin multiplexing for QSPI\n");
		return(1);
	}

	qspim_path = ad48n_setup_qspim(qspim_device);
	if (qspim_path < 0) {
		goto abort;
	}

	if (write) {
		for (pcs = CS0; pcs <= CS3; pcs++) {
			ad48n_qspi_write(qspim_path, pcs, 0xff, 0x08);
		}
	}


	for (pcs = CS0; pcs <= CS3; pcs++) {
		char din = 0x08;
		char dout;
		u_int16 reg = 0xff;

		printf("Testing PCS%d...", pcs);

		dout = ad48n_qspi_read(qspim_path, pcs, reg);

		if (dout != din) {
			printf("FAIL!!!\nData on PCS%d does not match\n", pcs);
			printf("expected: %d\ngot: %d\n", din, dout);
			goto abort;
		}

		printf("OK\n");
	}

	M_close(gpio_path);
	M_close(qspim_path);

	return(0);

abort:
	M_close(gpio_path);
	return(1);
}
