#pragma once
#include "types.h"

u32 LoadFileToSh4Mem(u32 offset,char*file);
bool LoadFileToSh4Bootrom(wchar *szFile);
u32 LoadBinfileToSh4Mem(u32 offset,char*file);
bool LoadFileToSh4Flashrom(wchar *szFile);
bool SaveSh4FlashromToFile(wchar *szFile);
