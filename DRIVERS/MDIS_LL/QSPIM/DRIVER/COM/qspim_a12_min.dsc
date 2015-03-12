#************************** MDIS4 device descriptor *************************
#
#        Author: kp
#         $Date: 2001/04/11 10:22:57 $
#     $Revision: 2.1 $
#
#   Description: Metadescriptor for A12 QSPIM
#
#****************************************************************************

QSPIM_A12_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   QSPIM_A12   # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   A12         # device name of baseboard
    DEVICE_SLOT      = U_INT32  0x1000      # onboard pseudo slot

    DEBUG_LEVEL      = U_INT32  0xc0008003  # LL driver
    DEBUG_LEVEL_MK   = U_INT32  0xc0008000  # MDIS kernel
    DEBUG_LEVEL_OSS  = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC = U_INT32  0xc0008000  # DESC calls

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
	IRQ_ENABLE		= U_INT32 1
}	
