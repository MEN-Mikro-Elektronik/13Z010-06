#***************************  M a k e f i l e  *******************************
#
#         Author: ag
#          $Date: 2010/05/10 15:02:14 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the XC7 QSPI example program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2010/05/10 15:02:14  ag
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2010 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=qspim_xc7

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\

MAK_INCL=$(MEN_INC_DIR)/qspim_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/usr_oss.h	\

MAK_INP1=qspim_xc7$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
