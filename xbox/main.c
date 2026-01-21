//Demo 01-Pong (hardware accelerated)

#include <hal/input.h>
#include <hal/xbox.h>
#include <hal/video.h>
#include <openxdk/debug.h>


#include "string.h"
#include "stdio.h"
#include <stdlib.h>


void XBoxStartup(void)
{
	int		i;

  debugPrint("nullDC for teh boxes ? wai~\n");

	XSleep(2000);
	XReboot();
}
