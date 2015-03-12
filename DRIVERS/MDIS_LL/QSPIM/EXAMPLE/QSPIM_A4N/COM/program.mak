#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2006/03/01 20:49:24 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for the QSPIM example program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.2  2006/03/01 20:49:24  cs
#   cosmetics for MDIS4/2004 compliancy
#
#   Revision 1.1  2000/09/25 13:24:12  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=qspim_a4n

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\

MAK_INCL=$(MEN_INC_DIR)/qspim_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/usr_oss.h	\

MAK_INP1=qspim_a4n$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
