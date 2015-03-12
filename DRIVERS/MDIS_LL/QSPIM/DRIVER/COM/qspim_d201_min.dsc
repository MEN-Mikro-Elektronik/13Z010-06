#************************** MDIS4 device descriptor *************************
#
#        Author: kp
#         $Date: 2001/04/11 10:22:58 $
#     $Revision: 2.1 $
#
#   Description: Metadescriptor for D201 QSPIM
#
#****************************************************************************

QSPIM_D201_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   QSPIM_D201  # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   F1_CPCI     # device name of baseboard
    DEVICE_SLOT      = U_INT32  2           # used slot on baseboard (0..n)

    DEBUG_LEVEL      = U_INT32  0xc0008003  # LL driver
    DEBUG_LEVEL_MK   = U_INT32  0xc0008000  # MDIS kernel
    DEBUG_LEVEL_OSS  = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC = U_INT32  0xc0008000  # DESC calls

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
	PCI_VENDOR_ID	 = U_INT32	0x10b5
	PCI_DEVICE_ID 	 = U_INT32	0x9050

	PCI_BASEREG_ASSIGN_0	= U_INT32 0
	PCI_BASEREG_ASSIGN_1	= U_INT32 2

	IRQ_ENABLE		= U_INT32 1
	
	PLD_CLOCK		= U_INT32 32000000
}	
