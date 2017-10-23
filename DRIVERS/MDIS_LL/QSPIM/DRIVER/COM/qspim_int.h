/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: qspim_int.h
 *
 *       Author: kp
 *
 *  Description: Internal header file for the QSPIM driver
 *
 *     Switches: see qspim_drv.c
 *
 *
 ****************************************************************************/

#ifndef _QSPIM_INT_H
#define _QSPIM_INT_H

#ifdef __cplusplus
	extern "C" {
#endif

#define _NO_LL_HANDLE		/* ll_defs.h: don't define LL_HANDLE struct */

#if defined(OS9000)
# define _OPT_PROTOS
#endif

#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */

#ifdef QSPIM_D201_SW
#include <MEN/pld_load.h>
#endif
#if defined(OS9000)
# include <regs.h>
#endif
/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general */
#define CH_NUMBER			1			/* number of device channels */
#define USE_IRQ				TRUE		/* interrupt required  */

#ifdef QSPIM_D201_SW
# define ADDRSPACE_COUNT		2		/* nr of required address spaces */
#endif

#if defined(QSPIM_SUPPORT_8240_DMA) || defined (QSPIM_SUPPORT_A21_DMA)
# define ADDRSPACE_COUNT		3		/* nr of required address spaces */
#else
# define ADDRSPACE_COUNT		1		/* nr of required address spaces */
#endif
/* debug settings */
#define DBG_MYLEVEL			h->dbgLevel
#define DBH					h->dbgHdl

#define ADDR_SIZE_SPC_1    		0x100
#define ADDR_SIZE_SPC_2    		0x100
/* GET_PARAM - get parameter from descriptor */
#define GET_PARAM(key,def,var)\
		if ((error = DESC_GetUInt32(h->descHdl, (def), \
									(u_int32 *)&var, \
									key)) &&\
			error != ERR_DESC_KEY_NOTFOUND){\
			    return( Cleanup(h,error) );\
        }

#define NOT_ALLOWED_WHEN_RUNNING \
 if( h->running ) { error = ERR_LL_DEV_BUSY; break; }

/*--- QSPI regs ---*/
#if defined QSPIM_D201_SW
/*---- D201 implementation of QSPI (16 bit register structure) ---*/
# define QSPI_QMCR		0x0000	/* (w) */
# define QSPI_QILR 		0x0002	/* (b) ????? offset in orginial QSPI: 4 */
# define QSPI_QIVR 		0x0003	/* (b) ????? offset in orginial QSPI: 5 */
# define QSPI_TIML		0x0010	/* (w) timer low */
# define QSPI_TIMH		0x0012	/* (w) timer high */
# define QSPI_QPDR		0x0014	/* (w) */
# define QSPI_QPAR		0x0016	/* (b) */
# define QSPI_QDDR		0x0017	/* (b) */
# define QSPI_SPCR0		0x0018  /* (w) */
# define QSPI_SPCR1		0x001A  /* (w) */
# define QSPI_SPCR2		0x001C  /* (w) */
# define QSPI_SPCR3		0x001E	/* (b) */
# define QSPI_SPSR		0x001F	/* (b) */
# define QSPI_SPCR4		0x0020	/* (w) */

#elif defined QSPIM_Z076
/* Z076 implementation of QSPI ---*/
# define QSPI_QMCR		0x0000	/* (w) */
# define QSPI_QILR 		0x0005	/* (b) */
# define QSPI_QIVR 		0x0004	/* (b) */
# define QSPI_TIMER		0x0010	/* (w) cycle timer period register */
/* note: the following long-word is differently swapped */
# define QSPI_QPDR		0x0016	/* (w) */
# define QSPI_QPAR		0x0015	/* (b) */
# define QSPI_QDDR		0x0014	/* (b) */
# define QSPI_SPCR0		0x0018  /* (w) */
# define QSPI_SPCR1		0x001A  /* (w) */
# define QSPI_SPCR2		0x001C  /* (w) */
# define QSPI_SPCR3		0x001F	/* (b) */
# define QSPI_SPSR		0x001E	/* (b) */
# define QSPI_SPCR4		0x0020	/* (w) */
#elif defined QSPIM_Z076_EM1A
/* EM1A05 Register layout equal to A12 but define here for clarification */
# define QSPI_QMCR      0x0002
# define QSPI_QILR      0x0006
# define QSPI_QIVR      0x0007
# define QSPI_TIMER     0x0012
# define QSPI_QPDR      0x0014
# define QSPI_QPAR      0x0016
# define QSPI_QDDR      0x0017
# define QSPI_SPCR0     0x001a
# define QSPI_SPCR1     0x0018
# define QSPI_SPCR2     0x001e
# define QSPI_SPCR3     0x001c
# define QSPI_SPSR      0x001d
# define QSPI_SPCR4     0x0022
#else
/*---- A12 implementation of QSPI (32 bit register structure) ---*/
# define QSPI_QMCR		0x0002	/* (w) */
# define QSPI_QILR 		0x0006	/* (b) */
# define QSPI_QIVR 		0x0007	/* (b) */
# define QSPI_TIMER		0x0012	/* (w) cycle timer period register */
/* note: the following long-word is differently swapped */
# define QSPI_QPDR		0x0014	/* (w) */
# define QSPI_QPAR		0x0016	/* (b) */
# define QSPI_QDDR		0x0017	/* (b) */
# define QSPI_SPCR0		0x001A  /* (w) */
# define QSPI_SPCR1		0x0018  /* (w) */
# define QSPI_SPCR2		0x001E  /* (w) */
# define QSPI_SPCR3		0x001C	/* (b) */
# define QSPI_SPSR		0x001D	/* (b) */
# define QSPI_SPCR4		0x0022	/* (w) */
#endif


/* standard QSPI queues (16 entries each) */
#define QSPI_RECRAM		0x0100	/* (w) */
#define QSPI_TRANRAM	0x0120	/* (w) */
#define QSPI_COMDRAM	0x0140	/* (b) */

/* extended QSPI queues (up to 256 entries each) */
#define QSPI_ERECRAM	0x0200	/* (w) */
#define QSPI_ETRANRAM	0x0400	/* (w) */
#define QSPI_ECOMDRAM	0x0600	/* (b) */

/*--- LL_HANDLE.xmtBufState ---*/
#define XMT_EMPTY	0
#define XMT_FILLING	1
#define XMT_FILLED	2
#define XMT_DOUBLE	3

/*--- QPDR shadow update ---*/
#define UPDATE_QPDR( val ) \
{ h->qpdrShadow = val; MWRITE_D16( h->maQspi, QSPI_QPDR, h->qpdrShadow ); }

/*--- Real-Time violation bit in QSPI_SPSR ---*/
#define QSPIM_RTV (1 << 4)

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* low-level handle */
typedef struct {
	/* general */
    int32           memAlloc;		/* size allocated for the handle */
    OSS_HANDLE      *osHdl;         /* oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /* irq handle */
    DESC_HANDLE     *descHdl;       /* desc handle */
#ifdef QSPIM_D201_SW
    MACCESS         maPlx;          /* hw access handle (PLX) */
#endif
    MACCESS         maQspi;         /* hw access handle (QSPI) */

#if defined(QSPIM_SUPPORT_8240_DMA) || defined (QSPIM_SUPPORT_A21_DMA)
	MACCESS         maDMA;         /* hw access handle (Z62 DMA) */
	MACCESS         maSRAM;         /* hw access handle (Z24 SRAM) */
#endif
	MDIS_IDENT_FUNCT_TBL idFuncTbl;	/* id function table */
	OSS_SEM_HANDLE *devSemHdl;		/* device semaphore handle */
	/* debug */
    u_int32         dbgLevel;		/* debug level */
	DBG_HANDLE      *dbgHdl;        /* debug handle */
	/* misc */
    u_int32         irqCount;       /* interrupt counter */
	u_int32			pldLoad;		/* load PLD in init */
	u_int32			qspiQueueLen; 	/* max entries used in queue */
	u_int32			frmLen;			/* bytes used in QSPI rcv/xmt ram */

	int32			pldClock;		/* QSPI PLD clock in Hz */
	int32			running;		/* timer/qspi is running */
	int32			timerValue;		/* shadow reg of QSPI_TIMER */

	u_int8			pcsDefState; 	/* default state of PCS pins */
	u_int8			irqEnabled;		/* Irq enabled flag */
	u_int16			qpdrShadow; 	/* shadow reg of QPDR */

	int32			errors;			/* QSPI error accumulator */
	u_int8			noFrameDup;		/* 1=prevent frame duplication */

	/* receive fifo (each entry has max. qspiQueueLen words) */
	int32			rcvFifoDepth;	/* max number of entries in rcv fifo */
	int32			rcvFifoCount; 	/* current number of entries in rcv fifo */
	u_int8			*rcvFifoNxtIn; 	/* ptr to next entry to be filled */
	u_int8			*rcvFifoNxtOut;	/* ptr to next entry to be read by app */
	u_int8			*rcvFifoStart;  /* ptr to start of array */
	u_int8			*rcvFifoEnd;  	/* ptr to end of array + 1*/
	u_int32			rcvFifoAlloc; 	/* memory allocated for rcv fifo */
	u_int8			*rcvOverBuf; 	/* used when receive fifo full */
	u_int32			rcvOverAlloc; 	/* memory allocated for rcvOverBuf */
	u_int32			doBlockRead; 	/* perform blocking read */
	u_int32 		doAutoMode;		/* perform automatic sending */
	OSS_SEM_HANDLE  *readSemHdl;
	int32			timerStarted; 	/* Is the QSPIM timer started? */

	/* transmit buffer */
	u_int8			*xmtBuf;		/* ptr to xmit buffer */
	int32			xmtBufState; 	/* see defs above */
	u_int32			xmtAlloc;		/* memory allocated for xmtBuf */

#if defined(QSPIM_SUPPORT_8240_DMA) || defined (QSPIM_SUPPORT_A21_DMA)
	/*---------------------------------------------------+  
	| DMA receive buffer                                 |
	|                                                    |
	| The DMA receive buffer is placed in the LL_HANDLE  |
	| so we can configure the DMA Buffer descriptors     |
	| once and only btach execute the DMA transfers in   |
	| the interupt handler. This saves us 8 PCIe single  |
	| long word writes.                                  |
	+---------------------------------------------------*/
	u_int8 *recvBuf;			/* ptr to recv buffer */
	u_int32 recvAlloc;			/* memory allocated for the recv buffer */
#endif
	/* signal handle */
	OSS_SIG_HANDLE	*frmSig;		/* QSPI frame finished signal */
	OSS_SIG_HANDLE	*emgSig;		/* emergency stop signal */

	/* callback function */
	/* type must be in sync with typedef QSPIM_CALLBACK_FUNCP in qspim_drv.h */
	void 			(*callbackFunc)(void *arg,u_int16 *qspiFrame,
									u_int32 qspiFrameLen ); /* function pointer */
	void			*callbackArg; 	/* argument to function */
	void			*callbackStatics; /* OS-9: static storage ptr for callbk */

#if defined(QSPIM_SUPPORT_8240_DMA) 
	MACCESS			dmaMa;			/* embedded utility block MACCESS */
#endif
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low-level driver jump table  */
#include <MEN/qspim_drv.h>   /* QSPIM driver header file */

#define __LoadD201Pld		QSPIM_GLOBNAME(QSPIM_VARIANT,LoadD201Pld)
#define __QSPIM_PldData 	QSPIM_GLOBNAME(QSPIM_VARIANT,PldData)
#define __QSPIM_PldIdent 	QSPIM_GLOBNAME(QSPIM_VARIANT,PldIdent)
#define __DMA_Init		 	QSPIM_GLOBNAME(QSPIM_VARIANT,DMA_Init)
#define __DMA_Transfer	 	QSPIM_GLOBNAME(QSPIM_VARIANT,DMA_Transfer)

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
extern const u_int8 __QSPIM_PldData[];

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
extern char* __QSPIM_PldIdent(void);
extern int __LoadD201Pld(LL_HANDLE *h);

#if defined(QSPIM_SUPPORT_8240_DMA) || defined(QSPIM_SUPPORT_A21_DMA)
extern void __DMA_Init(LL_HANDLE *h);
#endif

#if defined(QSPIM_SUPPORT_8240_DMA)
extern int32 __DMA_Transfer(LL_HANDLE *h,void *src, void *dst, u_int32 len, u_int32 dir);
#endif

#if defined(QSPIM_SUPPORT_A21_DMA)
extern int32 __DMA_Transfer(LL_HANDLE *h);
extern void __DMA_UpdateSize(LL_HANDLE *h);
#endif

#ifdef __cplusplus
	}
#endif

#endif	/* _QSPIM_INT_H */

