/***********************  I n c l u d e  -  F i l e  ************************
 *
 *         Name: qspim_drv.h
 *
 *       Author: kp
 *
 *  Description: Header file for QSPIM driver
 *               - QSPIM specific status codes
 *               - QSPIM function prototypes
 *
 *     Switches: _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *
 *
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
#define QSPIM_WOMQ				(M_DEV_OF+0x00) /* G,S: QSPI pins wired or */
#define QSPIM_BITS				(M_DEV_OF+0x01) /* G,S: bits per word */
#define QSPIM_CPOL				(M_DEV_OF+0x02) /* G,S: clock polarity */
#define QSPIM_CPHA				(M_DEV_OF+0x03) /* G,S: clock phase */
#define QSPIM_BAUD				(M_DEV_OF+0x04) /* G,S: baudrate */
#define QSPIM_DSCLK				(M_DEV_OF+0x05) /* G,S: PCS to SCLK delay */
#define QSPIM_DTL				(M_DEV_OF+0x06) /* G,S: Delay after transfer */
#define QSPIM_PCS_DEFSTATE		(M_DEV_OF+0x07) /* G,S: PCS inactive state */
#define QSPIM_TIMER_CYCLE_TIME	(M_DEV_OF+0x08) /* G,S: timer high time */
#define QSPIM_BLOCKING_READ		(M_DEV_OF+0x09)	/*  ,S: Use blocking read */
#define QSPIM_RCV_FIFO_DEPTH	(M_DEV_OF+0x0a) /* G,S: fifo depth */

/*--- Action/query codes ---*/
#define QSPIM_TIMER_STATE		(M_DEV_OF+0x10) /* G,S: start/stop timer/qspi */
#define QSPIM_FRM_SIG_SET		(M_DEV_OF+0x11) /*   S: install frame signal */
#define QSPIM_FRM_SIG_CLR		(M_DEV_OF+0x12) /*   S: remove frame signal */
#define QSPIM_EMG_SIG_SET		(M_DEV_OF+0x13) /*   S: install emergency signal */
#define QSPIM_EMG_SIG_CLR		(M_DEV_OF+0x14) /*   S: remove emergency signal */
#define QSPIM_CLR_RCV_FIFO		(M_DEV_OF+0x15) /*   S: clear receive fifo */
#define QSPIM_RCV_FIFO_COUNT	(M_DEV_OF+0x16) /* G  : entries in fifo */
#define QSPIM_ERRORS			(M_DEV_OF+0x17) /* G,S: query errors */
#define QSPIM_FRAMESYN			(M_DEV_OF+0x18)	/*   S: state of FRAMESYN signal */
#define QSPIM_NO_DUPLICATION	(M_DEV_OF+0x19) /*   S: prevent frame duplication */
#define QSPIM_AUTOMODE			(M_DEV_OF+0x1a)	/* G,S: enable automode */

/* QSPIM specific status codes (BLK) */	  					/* S,G: S=setstat, G=getstat */
#define QSPIM_BLK_DEFINE_FRM			(M_DEV_BLK_OF+0x00) /* S  : define frame */
#define QSPIM_BLK_DIRECT_WRITE_FUNC		(M_DEV_BLK_OF+0x01) /*   G: direct write function */
#define QSPIM_BLK_CALLBACK				(M_DEV_BLK_OF+0x02) /* S,G: install/remove callback */
#define QSPIM_BLK_DIRECT_ISETSTAT_FUNC	(M_DEV_BLK_OF+0x03) /*   G: direct setstat from interrupt */

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

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
 /* we have an MDIS4 men_types.h and mdis_api.h included */
 /* only 32bit compatibility needed!                     */
 #define INT32_OR_64  int32
 #define U_INT32_OR_64 u_int32
 typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */


#ifdef __cplusplus
      }
#endif

#endif /* _QSPIM_DRV_H */
