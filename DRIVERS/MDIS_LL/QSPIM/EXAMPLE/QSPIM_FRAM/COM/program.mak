#***************************  M a k e f i l e  *******************************
#
#         Author: michael.roth@men.de
#          $Date: 2014/03/28 13:54:30 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the QSPI FRAM test tool
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2014/03/28 13:54:30  MRoth
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2014 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=qspim_fram

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX) \
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)  \
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/qspim_drv.h \
         $(MEN_INC_DIR)/men_typs.h  \
         $(MEN_INC_DIR)/mdis_api.h  \
         $(MEN_INC_DIR)/usr_oss.h   \

MAK_INP1=qspim_fram$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
