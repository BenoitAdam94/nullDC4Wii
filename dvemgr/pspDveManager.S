	.set noreorder

#include "pspstub.s"

	STUB_START "pspDveManager",0x40090000,0x00030005
	STUB_FUNC  0x2ACFCB6D,pspDveMgrCheckVideoOut
	STUB_FUNC  0xF9C86C73,pspDveMgrSetVideoOut
	STUB_FUNC  0xDC4FADA7,pspSetVramSize
	STUB_END
