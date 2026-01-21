#include "types.h"
#include <windows.h>
#pragma comment(lib,"winmm.lib")

int os_GetFile(char *szFileName, char *szParse,u32 flags)
{
	static OPENFILENAME ofn;
	static char szFile[512]="";
	memset(&ofn,0, sizeof(OPENFILENAME));
	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= 0;
	ofn.lpstrFile		= szFileName;
	ofn.nMaxFile		= MAX_PATH;
	ofn.lpstrFilter		= szParse;
	ofn.nFilterIndex	= 1;
	ofn.nMaxFileTitle	= 128;
	ofn.lpstrFileTitle	= szFile;
	ofn.lpstrInitialDir	= NULL;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if(GetOpenFileName(&ofn)<=0)
	{
		DWORD err= CommDlgExtendedError();
		if (err==FNERR_INVALIDFILENAME)
		{
			szFileName[0]=0;
			if(GetOpenFileName(&ofn)<=0)
				return false;
			else
				return true;
		}
		return false;
	}
	return true;
}


int os_msgbox(const wchar* text,unsigned int type)
{
	return MessageBox(0,text,VER_FULLNAME,type);
}

int main(int argc, wchar* argv[])
{
	int rv=EmuMain(argc,argv);

	return rv;
}

double os_GetSeconds()
{
	return timeGetTime()/1000.0;
}