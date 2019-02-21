#***************************  M a k e f i l e  *******************************
#
#         Author: cs
#          $Date: 2015/03/05 18:08:52 $
#      $Revision: 2.2 $
#
#    Description: Makefile definitions for QSPIM (16Z076_QSPI) driver
#                 swapped mode
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2006-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************

MAK_NAME=z76_qspim_dma_sw

MAK_SWITCH= $(SW_PREFIX)MAC_MEM_MAPPED \
			$(SW_PREFIX)MAC_BYTESWAP \
			$(SW_PREFIX)QSPIM_Z076 \
			$(SW_PREFIX)QSPIM_VARIANT=Z76_QSPI_SW \
			$(SW_PREFIX)QSPIM_SUPPORT_A21_DMA

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)	\
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
         $(MEN_INC_DIR)/modcom.h	\
		 $(MEN_MOD_DIR)/qspim_int.h \
		 $(MEN_MOD_DIR)/qspim_int.h \

MAK_INP1=qspim_drv$(INP_SUFFIX)
MAK_INP2=dma_z62$(INP_SUFFIX)
MAK_INP3=

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
		$(MAK_INP3)

