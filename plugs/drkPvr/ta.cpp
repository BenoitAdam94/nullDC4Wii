#include "ta.h"
//Tile Accelerator state machine
//#include <emmintrin.h>


namespace TASplitter
{
	//Splitter function (normaly ta_dma_main , modified for split dma's)
	TaListFP* TaCmd;
}
using namespace TASplitter;

void libPvr_TaSQ(u32* data)
{
	verify(TaCmd!=0);
	Ta_Dma* t=(Ta_Dma*)data; 
	TaCmd(t,t);
}
void libPvr_TaDMA(u32* data,u32 size)
{
	verify(TaCmd!=0);
	Ta_Dma* ta_data=(Ta_Dma*)data;
	Ta_Dma* ta_data_end=ta_data+size-1;

	do
	{
		ta_data =TaCmd(ta_data,ta_data_end);
	}
	while(ta_data<=ta_data_end);
}

namespace TASplitter
{
	//DMA from emulator :)

	void TA_ListCont()
	{

	}
	void TA_ListInit()
	{

	}
	void TA_SoftReset()
	{

	}
}
