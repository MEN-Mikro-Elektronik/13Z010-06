#***************************  M a k e f i l e  *******************************
#
#         Author: ag
#          $Date: 2010/05/10 14:21:20 $
#      $Revision: 2.3 $
#
#    Description: Makefile definitions for QSPIM (16Z076_QSPI for EM1A) driver
#                 swapped mode
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver_z76_em1a_sw.mak,v $
#   Revision 2.3  2010/05/10 14:21:20  amorbach
#   R: APB build failed
#   M: MAK_NAME corrected
#
#   Revision 2.2  2010/05/06 10:56:36  amorbach
#   R: MDVE check failed
#   M: QSPIM_VARIANT added
#
#   Revision 2.1  2010/04/30 15:03:30  ag
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
# (c) Copyright 2010 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
#*****************************************************************************

MAK_NAME=z76_qspim_em1a_sw

MAK_SWITCH= $(SW_PREFIX)MAC_MEM_MAPPED \
			$(SW_PREFIX)QSPIM_Z076_EM1A \
			$(SW_PREFIX)QSPIM_VARIANT=Z76_QSPI_EM1A_SW

# Removed:
#			$(SW_PREFIX)MAC_BYTE_SWAPPED \
# wrong usage of MAC_BYTESWAP !

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
MAK_INP2=
MAK_INP3=

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
		$(MAK_INP3)
