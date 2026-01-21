	.set noreorder

#include "pspstub.s"

	STUB_START "pspDveManager",0x40090000,0x00070005
	STUB_FUNC  0x2ACFCB6D,pspDveMgrCheckVideoOut
	STUB_FUNC  0xF9C86C73,pspDveMgrSetVideoOut
	STUB_FUNC  0xDC4FADA7,pspSetVramSize
	STUB_FUNC  0x60A41671,usrDebugProfilerEnable
    STUB_FUNC  0xDA92A4C9,usrDebugProfilerDisable
    STUB_FUNC  0x42919B17,usrDebugProfilerClear
    STUB_FUNC  0xCC65DB8D,usrDebugProfilerGetRegs
	STUB_END
