#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2006/03/01 20:49:24 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for the QSPIM example program
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2000-2019, MEN Mikro Elektronik GmbH
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
