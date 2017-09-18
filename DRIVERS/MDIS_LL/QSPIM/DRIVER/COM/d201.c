/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: d201.c
 *      Project: QSPI for Mahr. Prototype phase with D201
 *
 *       Author: kp
 *        $Date: 2000/09/25 13:24:03 $
 *    $Revision: 1.1 $
 *
 *  Description: Load the D201 PLD with QSPI code
 *                      
 *                      
 *     Required: -
 *     Switches: QSPIM_D201_SW - D201 in swapped mode
 *
 *---------------------------[ Public Functions ]----------------------------
 *  
 *  
 *-------------------------------[ History ]---------------------------------
 *
 * $Log: d201.c,v $
 * Revision 1.1  2000/09/25 13:24:03  kp
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
#ifdef QSPIM_D201_SW
# define MAC_BYTESWAP
#endif

#include "qspim_int.h"
#include <MEN/pci9050.h>

/* PLD loader interface */
#define D201_PLD_DATA			MISC_TX_EPR
#define D201_PLD_DCLK			MISC_CLK_EPR
#define D201_PLD_CONFIG			(MISC_IO2_DATA | MISC_IO3_DATA | MISC_CS_EPR)
#define D201_PLD_STATUS			MISC_IO0_DATA
#define D201_PLD_CFGDONE		MISC_IO1_DATA

static void PLDCB_MsecDelay(void *arg, u_int32 msec);
static u_int8 PLDCB_GetStatus(void *arg);
static u_int8 PLDCB_GetCfgdone(void *arg);
static void PLDCB_SetData(void *arg, u_int8 state);
static void PLDCB_SetDclk(void *arg, u_int8 state);
static void PLDCB_SetConfig(void *arg, u_int8 state);

/********************************* LoadD201Pld *******************************
 *
 *  Description: Loads the PLD on the D201 with data
 *			   
 *			   
 *---------------------------------------------------------------------------
 *  Input......: h				low level handle
 *  Output.....: Returns:		error code
 *  Globals....: -
 ****************************************************************************/
int __LoadD201Pld( LL_HANDLE *h )
{
	int32 status;
	u_int32 val, size;
	MACCESS maRun = h->maPlx;
	u_int8  *dataP = (u_int8*)__QSPIM_PldData;   /* point to binary data */

	/* read+skip size */
	size  = (u_int32)(*dataP++) << 24;   		
	size |= (u_int32)(*dataP++) << 16;
	size |= (u_int32)(*dataP++) <<  8;
	size |= (u_int32)(*dataP++);

	/* set CS_EEPROM to 1 */
	MSETMASK_D32(maRun, RR_MISC, MISC_CS_EPR);

	/* set IO0...IO3 to USER I/O */
	MCLRMASK_D32( maRun, RR_MISC,
				  MISC_IO0_MODE | MISC_IO1_MODE | MISC_IO2_MODE | 
				  MISC_IO3_MODE);

	/* set IO0 and IO1 to Inputs */
	MCLRMASK_D32(maRun, RR_MISC, MISC_IO0_DIR | MISC_IO1_DIR);

	/* set IO2 and IO3 as userdefinable Outputs */
	MSETMASK_D32(maRun, RR_MISC, MISC_IO2_DIR | MISC_IO3_DIR);

	/* set IO2 and IO3 to 1 */
	MSETMASK_D32(maRun, RR_MISC, MISC_IO2_DATA | MISC_IO3_DATA);

	status = PLD_FLEX10K_LoadCallBk( dataP, size,
									 PLD_FIRSTBLOCK | PLD_LASTBLOCK,
									 (void*)h, PLDCB_MsecDelay,
									 PLDCB_GetStatus, PLDCB_GetCfgdone,
									 PLDCB_SetData, PLDCB_SetDclk,
									 PLDCB_SetConfig);

	/* set Clock_EEPROM to 1 */
	MSETMASK_D32(maRun, RR_MISC, MISC_CLK_EPR);

	/* set CS_EEPROM to 1 */
	MSETMASK_D32(maRun, RR_MISC, MISC_CS_EPR);

	/* set IO0...IO3 to WAITO, LLOCK, CS2, CS3 */
	MSETMASK_D32( maRun, RR_MISC,
				  MISC_IO0_MODE | MISC_IO1_MODE | MISC_IO2_MODE | 
				  MISC_IO3_MODE);

	/* reset local bus */
	MSETMASK_D32(maRun,RR_MISC,MISC_SOFTRES);
	OSS_Delay( h->osHdl, 10 );
	MCLRMASK_D32(maRun,RR_MISC,MISC_SOFTRES);

	/* error from PLD_FLEX10K_LoadCallBk() */
	if(status){		
		DBGWRT_ERR((DBH,"*** QSPIM: can't load PLD err=%d\n", status ));
		return status + ERR_PLD;
	}

    /*------------------------------+
    | enable global interrupt       |
    +------------------------------*/

    /* enable LINT1:module, LINT2:timeout, LINT:all */
    MSETMASK_D32( maRun, RR_INTCTL,
				  INTCTL_LINT1_ENABLE | INTCTL_LINT2_ENABLE | 
				  INTCTL_LINT_ENABLE );

	/* set bus width to 8 bit interface */
	val = MREAD_D32( maRun, RR_BUSREG(0));
	val = (val & ~0x00c00000);	/* bus with=8 */
	MWRITE_D32( maRun, RR_BUSREG(0), val );

	
    return 0;
}

/****************************** PLDCB_MsecDelay *****************************
 *
 *  Description:  Callback function for PLD_FLEX10K_LoadCallBk()
 *                delay 'msec' milli seconds
 *
 *---------------------------------------------------------------------------
 *  Input......:  arg       argument pointer        
 *                msec      milli seconds to delay  
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
static void PLDCB_MsecDelay(void *arg, u_int32 msec)		/* nodoc */
{
	LL_HANDLE *h = (LL_HANDLE *)arg;
	OSS_Delay( h->osHdl, msec);
}

/****************************** PLDCB_GetStatus *****************************
 *
 *  Description:  Callback function for PLD_FLEX10K_LoadCallBk()
 *                get setting of PLD STATUS bit
 *
 *---------------------------------------------------------------------------
 *  Input......:  arg       argument pointer        
 *  Output.....:  return    setting of PLD STATUS bit
 *  Globals....:  ---
 ****************************************************************************/
static u_int8 PLDCB_GetStatus(void *arg)		/* nodoc */
{
    u_int32 reg;
	LL_HANDLE *h = (LL_HANDLE *)arg;

    reg = MREAD_D32( h->maPlx, RR_MISC);

	if( D201_PLD_STATUS & reg )
	    return ( (u_int8) 1 );
	else
	    return ( (u_int8) 0 );
}

/****************************** PLDCB_GetCfgdone ****************************
 *
 *  Description:  Callback function for PLD_FLEX10K_LoadCallBk()
 *                get setting of PLD CONFIG DONE bit
 *
 *---------------------------------------------------------------------------
 *  Input......:  arg       argument pointer        
 *  Output.....:  return    setting of PLD CONFIG DONE bit
 *  Globals....:  ---
 ****************************************************************************/
static u_int8 PLDCB_GetCfgdone(void *arg)		/* nodoc */
{
    u_int32 reg;
	LL_HANDLE *h = (LL_HANDLE *)arg;

    reg = MREAD_D32( h->maPlx, RR_MISC);

    if( D201_PLD_CFGDONE & reg )
	    return ( (u_int8) 1 );
	else
	    return ( (u_int8) 0 );
}

/****************************** PLDCB_SetData *******************************
 *
 *  Description:  Callback function for PLD_FLEX10K_LoadCallBk()
 *                set PLD DATA bit
 *
 *---------------------------------------------------------------------------
 *  Input......:  arg       argument pointer        
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
static void PLDCB_SetData(void *arg, u_int8 state)		/* nodoc */
{
	LL_HANDLE *h = (LL_HANDLE *)arg;

    if(state)
        MSETMASK_D32( h->maPlx, RR_MISC, D201_PLD_DATA );
    else
        MCLRMASK_D32( h->maPlx, RR_MISC, D201_PLD_DATA );
}

/****************************** PLDCB_SetDclk *******************************
 *
 *  Description:  Callback function for PLD_FLEX10K_LoadCallBk()
 *                set PLD DCLK bit
 *
 *---------------------------------------------------------------------------
 *  Input......:  arg       argument pointer        
 *                state     bit setting             
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
static void PLDCB_SetDclk(void *arg, u_int8 state)		/* nodoc */
{
	LL_HANDLE *h = (LL_HANDLE *)arg;

    if(state)
        MSETMASK_D32( h->maPlx, RR_MISC, D201_PLD_DCLK );
    else
        MCLRMASK_D32( h->maPlx, RR_MISC, D201_PLD_DCLK );
}

/****************************** PLDCB_SetConfig *****************************
 *
 *  Description:  Callback function for PLD_FLEX10K_LoadCallBk()
 *                set PLD CONFIG bit
 *
 *---------------------------------------------------------------------------
 *  Input......:  arg       argument pointer        
 *                state     bit setting             
 *  Output.....:  ---
 *  Globals....:  ---
 ****************************************************************************/
static void PLDCB_SetConfig(void *arg, u_int8 state)		/* nodoc */
{
	LL_HANDLE *h = (LL_HANDLE *)arg;

    if(state)
        MSETMASK_D32( h->maPlx, RR_MISC, D201_PLD_CONFIG );
    else
        MCLRMASK_D32( h->maPlx, RR_MISC, D201_PLD_CONFIG );
}

