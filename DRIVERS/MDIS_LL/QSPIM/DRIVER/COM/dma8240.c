/*********************  P r o g r a m  -  M o d u l e ***********************
 *
 *         Name: dma8240.c
 *      Project: QSPI for Mahr.
 *
 *       Author: kp
 *        $Date: 2006/03/01 20:49:08 $
 *    $Revision: 1.2 $
 *
 *  Description: Functions for 8240 DMA
 *
 *
 *     Required: -
 *     Switches: -
 *
 *---------------------------[ Public Functions ]----------------------------
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2001 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
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

#ifdef __KERNEL__
# include <linux/types.h>
#else
# include <stdint.h>
#endif

#include "qspim_int.h"

/*
 * DMA0 register offsets
 * all registers must be accessed swapped and with 32 bit accesses
 */
#define DMA_DMR		0x1100		/* mode register */
#define DMA_DSR		0x1104		/* status register */
#define DMA_CDAR	0x1108		/* current descriptor address */
#define DMA_SAR		0x1110		/* source address */
#define DMA_DAR		0x1118		/* destination address */
#define DMA_BCR		0x1120		/* byte count register */

#define DMA_WRITE(ma,off,val)	MWRITE_D32(ma,off,OSS_SWAP32(val))
#define DMA_READ(ma,off)		OSS_Swap32(MREAD_D32(ma,off))

#define	PCI_CNF_ADR			0xFEC00000	/* PCI Configuration Address */
#define	PCI_DATA_ADR		0xFEE00000	/* PCI Data Address */
/*
 * DMA Mode  reg:
 * - Local memory delay count: 00 = 4 cycles
 * - Interrupt steer		 : 0 (don't care)
 * - Periodic DMA enable	 : 0 = disable
 * - DAH transfer size		 : 00 (don't care)
 * - SAH transfer size		 : 00 (don't care)
 * - DA hold enable			 : 0 = disable
 * - SA hold enable			 : 0 = disable
 * - PCI read command		 : 00 = PCI Read
 * - Error interrupt enable	 : 0 = disable
 * - End of transfer irq	 : 0 = disable
 * - descriptor location	 : 0 = local memory
 * - transfer mode			 : 1 = direct mode
 * - channel continue		 : 0
 * - channel start			 : set runtime
 */
#define DMR_VAL			0x00000004

static u_int32 PciReadCnf(		/* nodoc */
	OSS_HANDLE *oss,
	DBG_HANDLE *dbh,
	int32 busNbr,
	int32 devNbr,
	int32 function,
	int32 regIdx,
	int32 access);
static void PciWriteCnf(		/* nodoc */
	OSS_HANDLE *oss,
	DBG_HANDLE *dbh,
	int32 busNbr,
	int32 devNbr,
	int32 function,
	int32 regIdx,
	int32 access,
	u_int32 value);

/********************************* DMA_Init **********************************
 *
 *  Description: Init 8240 DMA
 *
 *
 *---------------------------------------------------------------------------
 *  Input......: h		ll handle
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
void __DMA_Init(LL_HANDLE *h)
{
	u_int32 embBase;

	DBGWRT_1((DBH,"8240 DMA Init\n"));

	/* get embedded utility block address */
	OSS_PciGetConfig( h->osHdl, 0, 0, 0, OSS_PCI_ACCESS_32 | 0x78,
					  (int32 *)&embBase );

	h->dmaMa = (MACCESS)(uintptr_t)embBase;

	DBGWRT_2((DBH," Embedded utility base=0x%lx\n", embBase));

	/* clear error detection reg 1 (ErrDr1) */
	OSS_PciSetConfig( h->osHdl, 0, 0, 0, OSS_PCI_ACCESS_8 | 0xc1, 0xff );

	/* clear status reg */
	DMA_WRITE( h->dmaMa, DMA_DSR, 0x93 );
}


/********************************* DMA_Transfer ******************************
 *
 *  Description: Perform DMA transfer
 *
 *			     Wait for it to finish
 *---------------------------------------------------------------------------
 *  Input......: h		ll handle
 *				 src    source address
 *				 dst	destination address
 *				 len	number of bytes to transfer
 *				 dir	direction
 *						0 = mem -> mem
 *						1 = mem -> pci
 *						2 = pci -> mem
 *						3 = pci -> pci
 *  Output.....: returns: 0=ok 1=unsuccessful
 *  Globals....: -
 ****************************************************************************/
int32 __DMA_Transfer(
	LL_HANDLE *h,
	void *src,
	void *dst,
	u_int32 len,
	u_int32 dir)
{
	u_int32 dmaStatus;
	int loopCnt=0;
	MACCESS ma = h->dmaMa;

	IDBGWRT_3((DBH,"DMA_Transfer 0x%x->0x%x 0x%x bytes, dir=%d\n",
			   src, dst, len, dir ));

	DMA_WRITE( ma, DMA_DMR, DMR_VAL );			/* set mode */
	DMA_WRITE( ma, DMA_SAR, (U_INT32_OR_64)src ); 	/* source address */
	DMA_WRITE( ma, DMA_DAR, (U_INT32_OR_64)dst ); 	/* dest. address */
	DMA_WRITE( ma, DMA_BCR, len ); 				/* byte count */
	DMA_WRITE( ma, DMA_CDAR, 0x10 | (dir<<1));  /* snoop enable, direction */

	/* START DMA transfer */
	DMA_WRITE( ma, DMA_DMR, DMR_VAL | 0x1 );	/* set CS bit */

	/*
	 * Wait for DMA to finish
	 * The documentation says that every access to the DMA control regs
	 * will slow down the DMA, so insert some delay
	 */
	while( (dmaStatus = DMA_READ( ma, DMA_DSR )) & 0x4 ){
		volatile int delay;
		for(delay=50; --delay>0; );
	    if( loopCnt++ > 10000 )
			break;
	}

	if( (dmaStatus = DMA_READ( ma, DMA_DSR )) & 0x4 ){
		DBGWRT_ERR((DBH,"*** DMA timeout\n"));
		return 1;
	}

	IDBGWRT_3((DBH, "DMA loops=%d dmaStat=%08x\n",
			   loopCnt, dmaStatus));
	if( dmaStatus & 0x90 ){
		int32 errDr1;
		errDr1 = PciReadCnf( h->osHdl, DBH, 0, 0, 0, 0xc1, 1 );
		IDBGWRT_ERR((DBH,"*** DMA error status=0x%x errDr1=0x%02x\n",
					 dmaStatus, errDr1));
		DMA_WRITE(ma, DMA_DSR, 0x90 );
		/* clear error detection reg 1 (ErrDr1) */
		PciWriteCnf( h->osHdl, DBH, 0, 0, 0, 0xc1, 1, errDr1 );
		return 1;
	}
	return 0;
}

/*
 * Following routines subsitute OSS_PciGetConfig/SetConfig since these
 * can't be used from interrupt routines
 * Only required in case of DMA errors
 */
static u_int32 PciReadCnf(		/* nodoc */
	OSS_HANDLE *oss,
	DBG_HANDLE *dbh,
	int32 busNbr,
	int32 devNbr,
	int32 function,
	int32 regIdx,
	int32 access)
{
	u_int32 retval=0;
	u_int32 cnf;
#ifdef OS9000
	u_int32 oldMask;
#endif

	/*--- build configuration address register ---*/
	cnf = 0x80000000 | ( busNbr<<16 ) | ( devNbr << 11 ) | ( function << 8 ) |
		  (regIdx & ~0x3);

	/* NOTE: physical addresses used here */

#ifdef OS9000
	oldMask = irq_maskget();	/* mask interrupts */
#endif

	*(volatile u_int32 *)PCI_CNF_ADR = OSS_SWAP32(cnf);
	retval = *(volatile u_int32 *)PCI_DATA_ADR;

#ifdef OS9000
	irq_restore( oldMask );		/* restore interrupt mask */
#endif


	retval = OSS_SWAP32(retval);


	switch( access ){
	case 1:
		/*--- byte access ---*/
		switch( regIdx & 0x3 ){
		case 0:	retval = retval & 0xff;					break;
		case 1:	retval = (retval & 0xff00) >> 8 ;		break;
		case 2:	retval = (retval & 0xff0000) >> 16;		break;
		case 3:	retval = (retval & 0xff000000) >> 24;	break;
		}
		break;
	case 2:
		/*--- word access ---*/
		switch( regIdx & 0x2 ){
		case 0:	retval = retval & 0xffff;				break;
		case 2: retval = (retval & 0xffff0000) >> 16; 	break;
		}
		break;
	}

	return retval;
}

static void PciWriteCnf(		/* nodoc */
	OSS_HANDLE *oss,
	DBG_HANDLE *dbh,
	int32 busNbr,
	int32 devNbr,
	int32 function,
	int32 regIdx,
	int32 access,
	u_int32 value)
{
	u_int32 readval;
	u_int32 cnf;
#ifdef OS9000
	u_int32 oldMask;
#endif

	/*--- build configuration address register ---*/
	cnf = 0x80000000 | ( busNbr<<16 ) | ( devNbr << 11 ) | ( function << 8 ) |
		  (regIdx & ~0x3);

	/* NOTE: physical addresses used here */

#ifdef OS9000
	oldMask = irq_maskget();	/* mask interrupts */
#endif

	*(volatile u_int32 *)PCI_CNF_ADR = OSS_SWAP32(cnf);

	if( access != 4 ){
		readval = *(volatile u_int32 *)PCI_DATA_ADR;
		readval = OSS_SWAP32(readval);
	} else {
		readval = value;
	}

	switch( access ){
	case 1:
		/*--- byte access ---*/
		switch( regIdx & 0x3 ){
		case 0:	readval = (readval & ~0xff) | value;	break;
		case 1:	readval = (readval & ~0xff00) | (value<<8) ;break;
		case 2:	readval = (readval & ~0xff0000) | (value<<16) ;break;
		case 3:	readval = (readval & ~0xff000000) | (value<<24) ;break;
		}
		break;
	case 2:
		/*--- word access ---*/
		switch( regIdx & 0x2 ){
		case 0:	readval = (readval & ~0xffff) | value;	break;
		case 2: readval = (readval & ~0xffff0000) | (value << 16); 	break;
		}
		break;
	}

	/*--- write back modified value ---*/
	*(volatile u_int32 *)PCI_DATA_ADR = OSS_SWAP32(readval);

#ifdef OS9000
	irq_restore( oldMask );		/* restore interrupt mask */
#endif
}



