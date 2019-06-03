#***************************  M a k e f i l e  *******************************
#
#         Author: ag/ts
#
#
#  Description: Makefile definitions for QSPIM (16Z076_QSPI for EM1A/EM10A) driver
#                 unswapped mode (customer specific design)
#
#
#  Attention: although this mak file is named _sw-mak it is NOT passing the
#             MAC_BYTE_SWAPPED define to MAK_SWITCH. This is because of the
#             special swapping situation in this IP core. MDIS Wizard always
#             adds the "_sw" for a .mak file for big endian targets like EM1A.
#             Furthermore, QSPIM_Z076_EM1A define isn't really used in the
#             driver. So the driver behaves effectively like the A12 default
#             register setup is running.
#
#*****************************************************************************

MAK_NAME=z76_qspim_em1a_sw
# the next line is updated during the MDIS installation
STAMPED_REVISION="13Z010-06_02_18-3-g2def2ea-dirty_2019-05-30"

DEF_REVISION=MAK_REVISION=$(STAMPED_REVISION)

MAK_SWITCH= $(SW_PREFIX)MAC_MEM_MAPPED \
		$(SW_PREFIX)$(DEF_REVISION) \
			$(SW_PREFIX)QSPIM_Z076_EM1A \
			$(SW_PREFIX)QSPIM_VARIANT=Z76_QSPI_EM1A_SW

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

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
		$(MAK_INP3)
