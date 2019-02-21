#***************************  M a k e f i l e  *******************************
#
#         Author: johannes.thumshirn@men.de
#          $Date: 2015/02/19 12:27:34 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the QSPI AD48N test tool
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2014-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************

MAK_NAME=qspim_ad48n

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX) \
                 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)  \
                 $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/qspim_drv.h \
         $(MEN_INC_DIR)/men_typs.h  \
         $(MEN_INC_DIR)/mdis_api.h  \
         $(MEN_INC_DIR)/usr_oss.h   \

MAK_INP1=qspim_ad48n$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
