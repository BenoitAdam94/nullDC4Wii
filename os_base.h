/*
	Basic OS functions
*/
int os_GetFile(char *szFileName, char *szParse=0,u32 flags=0);
int os_ExceptionHandler(void* info);

#define MBX_OK                       0x00000000L
#define MBX_OKCANCEL                 0x00000001L
#define MBX_ABORTRETRYIGNORE         0x00000002L
#define MBX_YESNOCANCEL              0x00000003L
#define MBX_YESNO                    0x00000004L
#define MBX_RETRYCANCEL              0x00000005L

#define MBX_ICONHAND                 0x00000010L
#define MBX_ICONQUESTION             0x00000020L
#define MBX_ICONEXCLAMATION          0x00000030L
#define MBX_ICONASTERISK             0x00000040L

#define MBX_USERICON                 0x00000080L
#define MBX_ICONWARNING              MBX_ICONEXCLAMATION
#define MBX_ICONERROR                MBX_ICONHAND

#define MBX_ICONINFORMATION          MBX_ICONASTERISK
#define MBX_ICONSTOP                 MBX_ICONHAND

#define MBX_DEFBUTTON1               0x00000000L
#define MBX_DEFBUTTON2               0x00000100L
#define MBX_DEFBUTTON3               0x00000200L

#define MBX_DEFBUTTON4               0x00000300L


#define MBX_APPLMODAL                0x00000000L
#define MBX_SYSTEMMODAL              0x00001000L
#define MBX_TASKMODAL                0x00002000L

#define MBX_HELP                     0x00004000L // Help Button


#define MBX_NOFOCUS                  0x00008000L
#define MBX_SETFOREGROUND            0x00010000L
#define MBX_DEFAULT_DESKTOP_ONLY     0x00020000L

#define MBX_TOPMOST                  0x00040000L
#define MBX_RIGHT                    0x00080000L
#define MBX_RTLREADING               0x00100000L

#define MBX_RV_OK                1
#define MBX_RV_CANCEL            2
#define MBX_RV_ABORT             3
#define MBX_RV_RETRY             4
#define MBX_RV_IGNORE            5
#define MBX_RV_YES               6
#define MBX_RV_NO                7

int os_msgbox(const char* text,unsigned int type);
double os_GetSeconds();