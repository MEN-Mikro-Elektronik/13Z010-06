#***************************  M a k e f i l e  *******************************
#
#         Author: cs
#          $Date: 2015/03/05 18:08:52 $
#      $Revision: 2.2 $
#
#    Description: Makefile definitions for QSPIM (16Z076_QSPI) driver
#                 swapped mode
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver_z76_dma_sw.mak,v $
#   Revision 2.2  2015/03/05 18:08:52  ts
#   R: built module name was wrong, not to distinguish from regular sw variant
#   M: changed mak_name to z76_qspim_dma_sw
#
#   Revision 2.1  2015/02/19 12:28:24  ts
#   Initial Revision
#
#   Revision 2.2  2010/04/30 14:36:19  ag
#   R:1. Wrong MAK_SWITCH: MAC_BYTE_SWAPPED
#   M:1. Replaced MAC_BYTE_SWAPPED by MAC_BYTESWAP
#
#   Revision 2.1  2006/03/01 20:49:13  cs
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
# (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
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

