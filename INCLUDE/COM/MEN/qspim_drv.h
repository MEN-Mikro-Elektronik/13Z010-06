/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: qspim_drv.h
 *
 *       Author: kp
 *        $Date: 2006/03/01 20:49:35 $
 *    $Revision: 2.4 $
 *
 *  Description: Header file for QSPIM driver
 *               - QSPIM specific status codes
 *               - QSPIM function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: qspim_drv.h,v $
 * Revision 2.4  2006/03/01 20:49:35  cs
 * cosmetics for MDIS4/2004 compliancy
 *
 * Revision 2.3  2001/05/25 11:09:25  kp
 * Added defs for direct isetstat function
 *
 * Revision 2.2  2001/04/11 10:23:10  kp
 * replace QSPIM_TIMER_HI/LO_TIME with QSPIM_TIMER_CYCLE_TIME
 * added QSPIM_FRAMESYN
 *
 * Revision 2.1  2000/09/25 13:24:19  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _QSPIM_DRV_H
#define _QSPIM_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif


/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/* Typedef for callback routine */
typedef void (*QSPIM_CALLBACK_FUNCP)(void *arg,u_int16 *qspiFrame,
									u_int32 qspiFrameLen );

/* Typedef for direct write function */
typedef int32 (*QSPIM_DIRECT_WRITE_FUNCP)(void *arg,u_int16 *qspiFrame,
										u_int32 qspiFrameLen );

/* Typedef for isetstat function */
typedef int32 (*QSPIM_DIRECT_ISETSTAT_FUNCP)(void *arg,int32 code,
											 int32 ch, int32 value );

/* structure to be passed to QSPIM_BLK_CALLBACK */
typedef struct {
	QSPIM_CALLBACK_FUNCP func;	/* callback routine to be called */
	void *arg;					/* argument to be passed to callback */
	void *statics;				/* OS-9 only: process static storage */
} QSPIM_CALLBACK_PARMS;


/* structure that is filled by QSPIM_BLK_DIRECT_WRITE_FUNC */
typedef struct {
	QSPIM_DIRECT_WRITE_FUNCP func; /* direct write function pointer */
	void *arg;					/* argument to be passed to direct write func*/
} QSPIM_DIRECT_WRITE_PARMS;

/* structure that is filled by QSPIM_BLK_DIRECT_ISETSTAT_FUNC */
typedef struct {
	QSPIM_DIRECT_ISETSTAT_FUNCP func; /* direct write function pointer */
	void *arg;					/* argument to be passed to direct isetstat func*/
} QSPIM_DIRECT_ISETSTAT_PARMS;



/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/

/*---- Configuration codes ---*/
/* QSPIM specific status codes (STD) */	  /* S,G: S=setstat, G=getstat */
#define QSPIM_WOMQ          (M_DEV_OF+0x00) /* G,S: QSPI pins wired or */
#define QSPIM_BITS			(M_DEV_OF+0x01) /* G,S: bits per word */
#define QSPIM_CPOL			(M_DEV_OF+0x02) /* G,S: clock polarity */
#define QSPIM_CPHA			(M_DEV_OF+0x03) /* G,S: clock phase */
#define QSPIM_BAUD			(M_DEV_OF+0x04) /* G,S: baudrate */
#define QSPIM_DSCLK			(M_DEV_OF+0x05) /* G,S: PCS to SCLK delay */
#define QSPIM_DTL			(M_DEV_OF+0x06) /* G,S: Delay after transfer */
#define QSPIM_PCS_DEFSTATE	(M_DEV_OF+0x07) /* G,S: PCS inactive state */
#define QSPIM_TIMER_CYCLE_TIME (M_DEV_OF+0x08) /* G,S: timer high time */
#define QSPIM_RCV_FIFO_DEPTH (M_DEV_OF+0x0a) /* G,S: fifo depth */

/*--- Action/query codes ---*/
#define QSPIM_TIMER_STATE	(M_DEV_OF+0x10) /* G,S: start/stop timer/qspi */
#define QSPIM_FRM_SIG_SET	(M_DEV_OF+0x11) /*   S: install frame signal */
#define QSPIM_FRM_SIG_CLR	(M_DEV_OF+0x12) /*   S: remove frame signal */
#define QSPIM_EMG_SIG_SET	(M_DEV_OF+0x13) /*   S: install emergency signal */
#define QSPIM_EMG_SIG_CLR	(M_DEV_OF+0x14) /*   S: remove emergency signal */
#define QSPIM_CLR_RCV_FIFO	(M_DEV_OF+0x15) /*   S: clear receive fifo */
#define QSPIM_RCV_FIFO_COUNT (M_DEV_OF+0x16) /* G  : entries in fifo */
#define QSPIM_ERRORS		(M_DEV_OF+0x17) /* G,S: query errors */
#define QSPIM_FRAMESYN		(M_DEV_OF+0x18)	/*   S: state of FRAMESYN signal */

/* QSPIM specific status codes (BLK) */	  		/* S,G: S=setstat, G=getstat */
#define QSPIM_BLK_DEFINE_FRM (M_DEV_BLK_OF+0x00) /* S  : define frame */
#define QSPIM_BLK_DIRECT_WRITE_FUNC (M_DEV_BLK_OF+0x01) /*   G: direct write function */
#define QSPIM_BLK_CALLBACK (M_DEV_BLK_OF+0x02) /* S,G: install/remove callback */
#define QSPIM_BLK_DIRECT_ISETSTAT_FUNC (M_DEV_BLK_OF+0x03) /*   G: direct setstat from interrupt */

/*--- Error flags queried with QSPIM_ERRORWS ---*/
#define QSPIM_ERR_RCV_FIFO_OVER		0x01	/* receive fifo overflow */


/*--- macros to make unique names for global symbols ---*/
#ifndef  QSPIM_VARIANT
# define QSPIM_VARIANT QSPIM
#endif

# define _QSPIM_GLOBNAME(var,name) var##_##name
#ifndef _ONE_NAMESPACE_PER_DRIVER_
# define QSPIM_GLOBNAME(var,name) _QSPIM_GLOBNAME(var,name)
#else
# define QSPIM_GLOBNAME(var,name) _QSPIM_GLOBNAME(QSPIM,name)
#endif

#define __QSPIM_GetEntry			QSPIM_GLOBNAME(QSPIM_VARIANT,GetEntry)

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void QSPIM_GetEntry(LL_ENTRY* drvP);
#endif
#endif /* _LL_DRV_ */


#ifdef __cplusplus
      }
#endif

#endif /* _QSPIM_DRV_H */
