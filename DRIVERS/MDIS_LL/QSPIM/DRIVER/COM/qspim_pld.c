/*********************  P r o g r a m  -  M o d u l e ***********************
 *  
 *         Name: qspim_pld.c
 *      Project: QSPIM module driver 
 *
 *       Author: kp
 *
 *  Description: PLD data array and ident function
 *                      
 *                      
 *     Required: -
 *     Switches: -
 *
 *---------------------------------------------------------------------------
 * Copyright (c) 1999-2019, MEN Mikro Elektronik GmbH
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
 
#include "qspim_int.h"		/* local prototypes */

/*--- INCLUDE PLD DATA HERE ---*/
#define char u_int8				/* huuupps.. ttf2arr always generates char..*/
#include "qspim_plddata.h"
#undef char

#ifdef QSPIM_PLD_VERSION
    static const char IdentString[]=MENT_XSTR_SFX(MAK_REVISION,QSPIM_PLD_VERSION);
#else
    static const char IdentString[]=MENT_XSTR(MAK_REVISION);
#endif


/* QSPIM_PldIdent: return ident string */
char* __QSPIM_PldIdent( void )
{
     return( (char*) IdentString );
}

