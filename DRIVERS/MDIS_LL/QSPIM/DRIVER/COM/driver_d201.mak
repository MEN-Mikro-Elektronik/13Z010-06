#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2010/05/06 10:55:47 $
#      $Revision: 1.2 $
#
#    Description: Makefile definitions for the QSPIM driver
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2000-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************

MAK_NAME=z76_qspim_d201

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
         $(SW_PREFIX)QSPIM_VARIANT=Z76_QSPIM_D201_SW \
         $(SW_PREFIX)QSPIM_D201_SW

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/pld$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)	\


MAK_INCL=$(MEN_INC_DIR)/qspim_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/oss.h		\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/maccess.h	\
         $(MEN_INC_DIR)/desc.h		\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_com.h	\
         $(MEN_INC_DIR)/ll_defs.h	\
         $(MEN_INC_DIR)/ll_entry.h	\
         $(MEN_INC_DIR)/dbg.h		\
         $(MEN_INC_DIR)/pci9050.h	\
		 $(MEN_INC_DIR)/pld_load.h  \
		 $(MEN_MOD_DIR)/qspim_int.h \
		 $(MEN_MOD_DIR)/qspim_plddata.h

MAK_INP1=qspim_drv$(INP_SUFFIX)
MAK_INP2=d201$(INP_SUFFIX)
MAK_INP3=qspim_pld$(INP_SUFFIX)

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
		$(MAK_INP3)

