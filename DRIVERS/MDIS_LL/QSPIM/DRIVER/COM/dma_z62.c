/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: dma_z62.c
 *      Project: QSPI for Mahr.
 *
 *       Author: jt
 *        $Date: 2015/02/19 12:28:21 $
 *    $Revision: 2.1 $
 *
 *  Description: Functions for z62 DMA
 *
 *
 *     Required: -
 *     Switches: -
 *
 *---------------------------[ Public Functions ]----------------------------
 *
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 2014-2019, MEN Mikro Elektronik GmbH
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

#include "qspim_int.h"

#ifndef BIT
#define BIT(x) (1 << (x))
#endif	/* BIT */

/* DMA Control/Status register */
#define DMASTA		0x0	/* DMA status register */
#define DMASTA_EN	BIT(0)	/* DMA enabled */
#define DMASTA_IEN	BIT(1)	/* DMA IRQ enabled */
#define DMASTA_IRQ	BIT(2)	/* DMA IRQ active */
#define DMASTA_ERR	BIT(3)	/* WBB error occured or DMA stopped manually */
#define DMASTA_ACT_BD	0xf0	/* Number of active buffer descriptors */

/* DMA Buffer Descriptros */
#define DMA_BD0_DAR		0x00	/* destination address */
#define DMA_BD0_SAR		0x04	/* source address */
#define DMA_BD0_LEN		0x08	/* Size */
#define DMA_BD0_CONF	0x0c	/* Buffer descriptor configuration */

#define DMA_BD1_DAR		0x10	/* destination address */
#define DMA_BD1_SAR		0x14	/* source address */
#define DMA_BD1_LEN		0x18	/* Size */
#define DMA_BD1_CONF	0x1c	/* Buffer descriptor configuration */

/* DMA Buffer Descriptor Configuration */
#define DMA_BD_NULL	BIT(0)	/* End of list */
#define DMA_BD_INC_DST	BIT(1)	/* Increment destination address */
#define DMA_BD_INC_SRC	BIT(2)	/* Increment source address */
#define DMA_BD_BRST_DST BIT(4)	/* Burst enable on destination */
#define DMA_BD_BRST_SRC BIT(5)	/* Burst enable on source */
#define DMA_BD_DST	(0x7 << 12) /* DMA destination config */
#define DMA_BD_DST_RAM_BD	0x1 /* DMA destination in BD */
#define DMA_BD_DST_RAM_A	0x2 /* DMA destination in RAM A*/
#define DMA_BD_DST_RAM_B	0x4 /* DMA destination in RAM B */
#define DMA_BD_SRC	(0x7 << 16) /* DMA source config */
#define DMA_BD_SRC_RAM_BD	0x1 /* DMA source in BD */
#define DMA_BD_SRC_RAM_A	0x2 /* DMA source in RAM A */
#define DMA_BD_SRC_RAM_B	0x4 /* DMA source in RAM B */

/********************************* DMA_SetupTx ******************************
 *
 *  Description: Setup DMA buffer descriptor for transmitting
 *
 *---------------------------------------------------------------------------
 *  Input......: h		ll handle
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void DMA_SetupTx(LL_HANDLE *h)
{
	u_int32 dmaCfg;
	u_int32 dstAddr;
	u_int32 srcAddr;
	u_int32 bdLen = (h->qspiQueueLen * 2) / sizeof(u_int32);

	dstAddr = (u_int32) QSPI_ETRANRAM; 

#if defined(LINUX)
	srcAddr = (u_int32) virt_to_bus(h->xmtBuf);
#else
 	srcAddr = (u_int32) h->xmtBuf;
#endif

	IDBGWRT_3((DBH,"DMA_SetupTx 0x%x->0x%x (%d bytes)\n",
			   srcAddr, dstAddr, bdLen));

	/* Write source address */
	MWRITE_D32(h->maSRAM, DMA_BD0_SAR, srcAddr);

	/* Write destination address */
	MWRITE_D32(h->maSRAM, DMA_BD0_DAR, dstAddr);

	/* Write len */
	MWRITE_D32(h->maSRAM, DMA_BD0_LEN, bdLen);

	/* Write DMA Config */
	dmaCfg = 0;
	dmaCfg |= (DMA_BD_SRC_RAM_A << 16) | (DMA_BD_DST_RAM_B << 12);
	dmaCfg |= DMA_BD_BRST_SRC | DMA_BD_BRST_DST;
	dmaCfg |= DMA_BD_INC_SRC | DMA_BD_INC_DST;

	MWRITE_D32(h->maSRAM, DMA_BD0_CONF, dmaCfg);
}

/********************************* DMA_SetupRx ******************************
 *
 *  Description: Setup DMA buffer descriptors for receiving
 *
 *---------------------------------------------------------------------------
 *  Input......: h		ll handle
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void DMA_SetupRx(LL_HANDLE *h)
{
	u_int32 dmaCfg = 0;
	u_int32 srcAddr = (u_int32) QSPI_ERECRAM;
#if defined(LINUX)
	u_int32 dstAddr = (u_int32) virt_to_bus(h->recvBuf);
#else
	u_int32 dstAddr = (u_int32) h->recvBuf;
#endif
	u_int32 bdLen = (h->qspiQueueLen * 2) / sizeof(u_int32);

	IDBGWRT_3((DBH,"DMA_SetupRx 0x%x->0x%x (%d bytes)\n",
			   srcAddr, dstAddr, bdLen));

	/* Write source address */
	MWRITE_D32(h->maSRAM, DMA_BD1_SAR, srcAddr);

	/* Write destination address */
	MWRITE_D32(h->maSRAM, DMA_BD1_DAR, dstAddr);

	/* Write len */
	MWRITE_D32(h->maSRAM, DMA_BD1_LEN, bdLen);

	/* Write DMA Config */
	dmaCfg |= (DMA_BD_SRC_RAM_B << 16) | (DMA_BD_DST_RAM_A << 12);
	dmaCfg |= DMA_BD_BRST_SRC | DMA_BD_BRST_DST;
	dmaCfg |= DMA_BD_INC_SRC | DMA_BD_INC_DST | DMA_BD_NULL;

	MWRITE_D32(h->maSRAM, DMA_BD1_CONF, dmaCfg);
}

#define ALIGN_LEN(x) ((x) & 0x3 ? (x) &= ~0x3, (x) + 4 : (x))
void __DMA_UpdateSize(LL_HANDLE *h)
{
	u_int32 bdLen = ALIGN_LEN(h->frmLen) / sizeof(u_int32);
	
	MWRITE_D32(h->maSRAM, DMA_BD0_LEN, bdLen);
	MWRITE_D32(h->maSRAM, DMA_BD1_LEN, bdLen);
}


/********************************* DMA_Init **********************************
 *
 *  Description: Init Z62 DMA
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: h		ll handle
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
void __DMA_Init(LL_HANDLE *h)
{
	DBGWRT_1((DBH,"z62 DMA Init\n"));

	DMA_SetupTx(h);
	DMA_SetupRx(h);

}

/********************************* DMA_Transfer ******************************
 *
 *  Description: Perform batch DMA transfer of both buffer descriptors 
 *               configured.
 *
 *			     Wait for it to finish
 *---------------------------------------------------------------------------
 *  Input......: h		ll handle
 *  Output.....: returns: 0=ok 1=unsuccessful
 *  Globals....: -
 ****************************************************************************/
int32 __DMA_Transfer(LL_HANDLE *h)
{

	int timeout = 100;
	u_int32 dmaSta;

	IDBGWRT_3((DBH,"DMA_Transfer\n"));

	/* Start DMA Transfer */
	MWRITE_D32(h->maDMA, DMASTA, DMASTA_EN);
		
	/* Poll for completion */
	while( (dmaSta = MREAD_D32(h->maDMA, DMASTA)) & DMASTA_EN ){
		volatile int delay = 50;
		while( delay-- );

		timeout--;
		if( timeout == 0 ){
			DBGWRT_ERR((DBH, "DMA_Transfer - timeout polling for DMA finish\n"));
			goto error;
		}
	}

	if( dmaSta & DMASTA_ERR) {
		DBGWRT_ERR((DBH, "DMA_Transfer - error on DMA transmission\n"));
		dmaSta &= ~DMASTA_ERR;
		MWRITE_D32(h->maDMA, DMASTA, dmaSta);
		goto error;
	}

	return 0;

error:
	return 1;
}
