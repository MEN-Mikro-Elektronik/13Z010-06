#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2015/02/19 12:28:23 $
#      $Revision: 2.1 $
#
#    Description: Makefile definitions for the QSPIM driver with 16z062 DMA
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2014-2019, MEN Mikro Elektronik GmbH
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

MAK_NAME=qspim_dma

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
           $(SW_PREFIX)QSPIM_USE_DMA   \
           $(SW_PREFIX)QSPIM_USE_DMA_Z62   \
           $(SW_PREFIX)QSPIM_VARIANT=QSPIM_A12 \
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

MAK_INP1=qspim_drv$(INP_SUFFIX)
MAK_INP2=dma_z62$(INP_SUFFIX)
MAK_INP3=

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2) \
		$(MAK_INP3)

