#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2000/09/25 13:24:15 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the QSPIM example program
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2000-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************

MAK_NAME=qspim_cb

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/os_men$(LIB_SUFFIX)	\
		 $(LIB_PREFIX)$(MEN_LIB_DIR)/drvsupp$(LIB_SUFFIX)	\
		 $(CPULIB)	

MAK_INCL=$(MEN_INC_DIR)/qspim_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_err.h	\

MAK_INP1=qspim_cb$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
