#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#
#    Description: Makefile definitions for the QSPIM driver
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2000-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

MAK_NAME=z76_qspim_d201
# the next line is updated during the MDIS installation
STAMPED_REVISION="13Z010-06_02_18-3-g2def2ea-dirty_2019-05-30"

DEF_REVISION=MAK_REVISION=$(STAMPED_REVISION)

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
		$(SW_PREFIX)$(DEF_REVISION) \
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

