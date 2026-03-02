#include "arm_aica.h"
#include "arm7.h"
#include "mem.h"
#include <math.h>




//Mainloop
void FASTCALL armUpdateARM(u32 Cycles)
{
	arm_Run(Cycles/arm_sh4_bias);
}
