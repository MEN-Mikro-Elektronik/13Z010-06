/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: qspim_pld.c
 *      Project: QSPIM module driver (MDIS V4.x)
 *
 *       Author: kp
 *        $Date: 2000/09/25 13:24:09 $
 *    $Revision: 1.1 $
 *
 *  Description: PLD data array and ident function
 *                      
 *                      
 *     Required: -
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany 
 ****************************************************************************/
 
#include "qspim_int.h"		/* local prototypes */

/*--- INCLUDE PLD DATA HERE ---*/
#define char u_int8				/* huuupps.. ttf2arr always generates char..*/
#include "qspim_plddata.h"
#undef char

/* QSPIM_PldIdent: return ident string */
char* __QSPIM_PldIdent( void )
{
     return( "QSPIM - QSPIM pld data "
			 QSPIM_PLD_VERSION
			 ": $Id: qspim_pld.c,v 1.1 2000/09/25 13:24:09 kp Exp $" ) ;
}

