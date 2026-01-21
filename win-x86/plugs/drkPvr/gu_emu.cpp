#include <windows.h>
#include "config.h"
#include "plugs/drkpvr/config.h"

#if REND_API == REND_PSP && HOST_OS != OS_PSP
#include "gu_emu.h"

#include <gl\glew.h>
#pragma comment( lib, "glew32.lib" )

#include <gl\gl.h>								// Header File For The OpenGL32 Library
#include <gl\glu.h>								// Header File For The GLu32 Library
//#include <gl\glaux.h>								// Header File For The GLaux Library
#undef near
#undef far
#define CRASHNOW __asm int 3;

typedef void (*GuSwapBuffersCallback)(void** display,void** render);

#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glu32.lib" )

HGLRC           hRC=NULL;							// Permanent Rendering Context
HDC             hDC=NULL;							// Private GDI Device Context
HWND            hWnd=NULL;							// Holds Our Window Handle
HINSTANCE       hInstance;							// Holds The Instance Of The Application

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);				// Declaration For WndProc


typedef void (APIENTRY *PFNWGLEXTSWAPCONTROLPROC) (int);
typedef int (*PFNWGLEXTGETSWAPINTERVALPROC) (void);
 
//declare functions
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
PFNWGLEXTGETSWAPINTERVALPROC wglGetSwapIntervalEXT = NULL;
 
//init VSync func
void InitVSync()
{
   //get extensions of graphics card
   char* extensions = (char*)glGetString(GL_EXTENSIONS);
  
   //is WGL_EXT_swap_control in the string? VSync switch possible?
   if (strstr(extensions,"WGL_EXT_swap_control"))
   {
      //get address's of both functions and save them
      wglSwapIntervalEXT = (PFNWGLEXTSWAPCONTROLPROC)
          wglGetProcAddress("wglSwapIntervalEXT");
      wglGetSwapIntervalEXT = (PFNWGLEXTGETSWAPINTERVALPROC)
          wglGetProcAddress("wglGetSwapIntervalEXT");
  }
}
 
bool VSyncEnabled()
{
   //if interval is positif, it is not 0 so enabled ;)
   return (wglGetSwapIntervalEXT() > 0);
}
 
void SetVSyncState(bool enable)
{
    if (enable)
       wglSwapIntervalEXT(1); //set interval to 1 -&gt; enable
    else
       wglSwapIntervalEXT(0); //disable
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

//	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
	dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		32,											// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window

	/*
	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}*/

#ifdef GLEW_OK
	GLenum glewe=glewInit();
	if (GLEW_OK !=glewe )
	{
		return false;
	}
#endif

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	InitVSync();
	SetVSyncState(false);
	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
			//	active=TRUE;						// Program Is Active
			}
			else
			{
			//	active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			//keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			//keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			//ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

/*
Some global tables
*/
GLint gu2gl_df[]=
{
	GL_NEVER,//GU_NEVER - No pixels pass the depth-test
	GL_ALWAYS,//GU_ALWAYS - All pixels pass the depth-test
	GL_EQUAL,//GU_EQUAL - Pixels that match the depth-test pass
	GL_NOTEQUAL,//GU_NOTEQUAL - Pixels that doesn't match the depth-test pass
	GL_LESS /*GL_GREATER/*GL_LESS*/,//GU_LESS - Pixels that are less in depth passes
	GL_LEQUAL/*GL_GEQUAL/*GL_LEQUAL*/,//GU_LEQUAL - Pixels that are less or equal in depth passes
	GL_GREATER/*GL_LESS/*GL_GREATER*/,//GU_GREATER - Pixels that are greater in depth passes
	GL_GEQUAL/*GL_LEQUAL/*GL_GEQUAL*/,//GU_GEQUAL - Pixels that are greater or equal passes
};
/** @addtogroup GU */
/*@{*/

/**
  * Set depth buffer parameters
  *
  * @param zbp - VRAM pointer where the depthbuffer should start
  * @param zbw - The width of the depth-buffer (block-aligned)
  * 
**/
void sceGuDepthBuffer(void* zbp, int zbw)
{
	verify(zbw==512);
	//CRASHNOW
	
}

/**
  * Set display buffer parameters
  *
  * @par Example: Setup a standard 16-bit display buffer
  * @code
  * sceGuDispBuffer(480,272,(void*)512*272*2,512)
{
	CRASHNOW
	CRASHNOW
	
} // 480*272, skipping the draw buffer located at address 0
  * @endcode
  *
  * @param width - Width of the display buffer in pixels
  * @param height - Width of the display buffer in pixels
  * @param dispbp - VRAM pointer to where the display-buffer starts
  * @param dispbw - Display buffer width (block aligned)
  *
**/
void sceGuDispBuffer(int width, int height, void* dispbp, int dispbw)
{
	verify(width==480 && height==272 && dispbw==512);
	//CRASHNOW
	
}

/**
  * Set draw buffer parameters (and store in context for buffer-swap)
  *
  * Available pixel formats are:
  *   - GU_PSM_5650
  *   - GU_PSM_5551
  *   - GU_PSM_4444
  *   - GU_PSM_8888
  *
  * @par Example: Setup a standard 16-bit draw buffer
  * @code
  * sceGuDrawBuffer(GU_PSM_5551,(void*)0,512)
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param psm - Pixel format to use for rendering (and display)
  * @param fbp - VRAM pointer to where the draw buffer starts
  * @param fbw - Frame buffer width (block aligned)
**/
void sceGuDrawBuffer(int psm, void* fbp, int fbw)
{
	//CRASHNOW
	printf("sceGuDrawBuffer(%d,%d,%d);\n",psm,fbp,fbw);
	
}

/**
  * Set draw buffer directly, not storing parameters in the context
  *
  * @param psm - Pixel format to use for rendering
  * @param fbp - VRAM pointer to where the draw buffer starts
  * @param fbw - Frame buffer width (block aligned)
**/
void sceGuDrawBufferList(int psm, void* fbp, int fbw)
{
	CRASHNOW
	
}

/**
  * Turn display on or off
  *
  * Available states are:
  *   - GU_TRUE (1) - Turns display on
  *   - GU_FALSE (0) - Turns display off
  *
  * @param state - Turn display on or off
  * @returns State of the display prior to this call
**/
int sceGuDisplay(int state)
{
	verify(state==1);
	//CRASHNOW
	return 1;	
}

/**
  * Select which depth-test function to use
  *
  * Valid choices for the depth-test are:
  *   - GU_NEVER - No pixels pass the depth-test
  *   - GU_ALWAYS - All pixels pass the depth-test
  *   - GU_EQUAL - Pixels that match the depth-test pass
  *   - GU_NOTEQUAL - Pixels that doesn't match the depth-test pass
  *   - GU_LESS - Pixels that are less in depth passes
  *   - GU_LEQUAL - Pixels that are less or equal in depth passes
  *   - GU_GREATER - Pixels that are greater in depth passes
  *   - GU_GEQUAL - Pixels that are greater or equal passes
  *
  * @param function - Depth test function to use
**/ 


void sceGuDepthFunc(int function)
{
	//CRASHNOW
	glDepthFunc(gu2gl_df[function]);
	
}

/**
  * Mask depth buffer writes
  *
  * @param mask - GU_TRUE(1) to disable Z writes, GU_FALSE(0) to enable
**/
void sceGuDepthMask(int mask)
{
	glDepthMask(mask?GL_FALSE:GL_TRUE);
	//CRASHNOW
	
}

void sceGuDepthOffset(unsigned int offset)
{
	CRASHNOW
	
}

/**
  * Set which range to use for depth calculations.
  *
  * @note The depth buffer is inversed, and takes values from 65535 to 0.
  *
  * Example: Use the entire depth-range for calculations:
  * @code
  * sceGuDepthRange(65535,0)
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param near - Value to use for the near plane
  * @param far - Value to use for the far plane
**/
void sceGuDepthRange(int _near, int _far)
{
	glDepthRange(_near/65535.f,_far/65535.f);
	//CRASHNOW
	
}

void sceGuFog(float near, float far, unsigned int color)
{
	CRASHNOW
	
}

/**
  * Initalize the GU system
  *
  * This function MUST be called as the first function, otherwise state is undetermined.
**/
void sceGuInit(void)
{
	//CRASHNOW
	CreateGLWindow("winGU",480,272,32,false);
	
}

/**
  * Shutdown the GU system
  *
  * Called when GU is no longer needed
**/
void sceGuTerm(void)
{
	//CRASHNOW
	KillGLWindow();
	
}

void sceGuBreak(int a0)
{
	CRASHNOW
	
}
void sceGuContinue(void)
{
	CRASHNOW
	
}

/**
  * Setup signal handler
  *
  * Available signals are:
  *   - GU_CALLBACK_SIGNAL - Called when sceGuSignal is used
  *   - GU_CALLBACK_FINISH - Called when display list is finished
  *
  * @param signal - Signal index to install a handler for
  * @param callback - Callback to call when signal index is triggered
  * @returns The old callback handler
**/
void* sceGuSetCallback(int signal, void (*callback)(int))
{
	CRASHNOW
	return 0;	
}

/**
  * Trigger signal to call code from the command stream
  *
  * Available behaviors are:
  *   - GU_BEHAVIOR_SUSPEND - Stops display list execution until callback function finished
  *   - GU_BEHAVIOR_CONTINUE - Do not stop display list execution during callback
  *
  * @param signal - Signal to trigger
  * @param behavior - Behavior type
**/
void sceGuSignal(int signal, int behavior)
{
	CRASHNOW
	
}

/**
  * Send raw float-command to the GE
  *
  * The argument is converted into a 24-bit float before transfer.
  *
  * @param cmd - Which command to send
  * @param argument - Argument to pass along
**/
void sceGuSendCommandf(int cmd, float argument)
{
	CRASHNOW
	
}

/**
  * Send raw command to the GE
  *
  * Only the 24 lower bits of the argument is passed along.
  *
  * @param cmd - Which command to send
  * @param argument - Argument to pass along
**/
void sceGuSendCommandi(int cmd, int argument)
{
	CRASHNOW
	
}

/**
  * Allocate memory on the current display list for temporary storage
  *
  * @note This function is NOT for permanent memory allocation, the
  * memory will be invalid as soon as you start filling the same display
  * list again.
  *
  * @param size - How much memory to allocate
  * @returns Memory-block ready for use
**/
void* sceGuGetMemory(int size)
{
	CRASHNOW
	return 0;
}

/**
  * Start filling a new display-context
  *
  * Contexts available are:
  *   - GU_DIRECT - Rendering is performed as list is filled
  *   - GU_CALL - List is setup to be called from the main list
  *   - GU_SEND - List is buffered for a later call to sceGuSendList()
  *
  * The previous context-type is stored so that it can be restored at sceGuFinish().
  *
  * @param cid - Context Type
  * @param list - Pointer to display-list (16 byte aligned)
**/
void sceGuStart(int cid, void* list)
{
	verify(cid==GU_DIRECT);
	//CRASHNOW
	
}

/**
  * Finish current display list and go back to the parent context
  *
  * If the context is GU_DIRECT, the stall-address is updated so that the entire list will
  * execute. Otherwise, only the terminating action is written to the list, depending on
  * context-type.
  *
  * The finish-callback will get a zero as argument when using this function.
  *
  * This also restores control back to whatever context that was active prior to this call.
  *
  * @returns Size of finished display list
**/
int sceGuFinish(void)
{
	//CRASHNOW
	glFinish();
	return 0;	
}

/**
  * Finish current display list and go back to the parent context, sending argument id for
  * the finish callback.
  *
  * If the context is GU_DIRECT, the stall-address is updated so that the entire list will
  * execute. Otherwise, only the terminating action is written to the list, depending on
  * context-type.
  *
  * @param id - Finish callback id (16-bit)
  * @returns Size of finished display list
**/
int sceGuFinishId(unsigned int id)
{
	CRASHNOW
	return 0;	
}

/**
  * Call previously generated display-list
  *
  * @param list - Display list to call
**/
void sceGuCallList(const void* list)
{
	CRASHNOW
	
}

/**
  * Set wether to use stack-based calls or signals to handle execution of called lists.
  *
  * @param mode - GU_TRUE(1) to enable signals, GU_FALSE(0) to disable signals and use
  * normal calls instead.
**/
void sceGuCallMode(int mode)
{
	CRASHNOW
	
}

/**
  * Check how large the current display-list is
  *
  * @returns The size of the current display list
**/
int sceGuCheckList(void)
{
	CRASHNOW
	return 0;	
}

/**
  * Send a list to the GE directly
  *
  * Available modes are:
  *   - GU_TAIL - Place list last in the queue, so it executes in-order
  *   - GU_HEAD - Place list first in queue so that it executes as soon as possible
  *
  * @param mode - Whether to place the list first or last in queue
  * @param list - List to send
  * @param context - Temporary storage for the GE context
**/
void sceGuSendList(int mode, const void* list, PspGeContext* context)
{
	CRASHNOW
	
}

/**
  * Swap display and draw buffer
  *
  * @returns Pointer to the new drawbuffer
**/
void* sceGuSwapBuffers(void)
{
	//CRASHNOW
	SwapBuffers(hDC);
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        // If the message is WM_QUIT, exit the while loop
        if (msg.message == WM_QUIT)
            break;

        // Translate the message and dispatch it to WindowProc()
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	return 0;	
}

/**
  * Wait until display list has finished executing
  *
  * @par Example: Wait for the currently executing display list
  * @code
  * sceGuSync(0,0)
{
	CRASHNOW
	
}
  * @endcode
  *
  * Available modes are:
  *   - GU_SYNC_WAIT
  *   - GU_SYNC_NOWAIT
  *
  * Available what are:
  *   - GU_SYNC_FINISH - Wait until the last sceGuFinish command is reached
  *   - GU_SYNC_SIGNAL - Wait until the last (?) signal is executed
  *   - GU_SYNC_DONE - Wait until all commands currently in list are executed
  *
  * @param mode - Whether to wait or not
  * @param what - What to sync to
  * @returns Unknown at this time
**/
int sceGuSync(int mode, int what)
{
	//CRASHNOW
	glFlush();
	return 0;	
}

/**
  * Draw array of vertices forming primitives
  *
  * Available primitive-types are:
  *   - GU_POINTS - Single pixel points (1 vertex per primitive)
  *   - GU_LINES - Single pixel lines (2 vertices per primitive)
  *   - GU_LINE_STRIP - Single pixel line-strip (2 vertices for the first primitive, 1 for every following)
  *   - GU_TRIANGLES - Filled triangles (3 vertices per primitive)
  *   - GU_TRIANGLE_STRIP - Filled triangles-strip (3 vertices for the first primitive, 1 for every following)
  *   - GU_TRIANGLE_FAN - Filled triangle-fan (3 vertices for the first primitive, 1 for every following)
  *   - GU_SPRITES - Filled blocks (2 vertices per primitive)
  *
  * The vertex-type decides how the vertices align and what kind of information they contain.
  * The following flags are ORed together to compose the final vertex format:
  *   - GU_TEXTURE_8BIT - 8-bit texture coordinates
  *   - GU_TEXTURE_16BIT - 16-bit texture coordinates
  *   - GU_TEXTURE_32BITF - 32-bit texture coordinates (float)
  *
  *   - GU_COLOR_5650 - 16-bit color (R5G6B5A0)
  *   - GU_COLOR_5551 - 16-bit color (R5G5B5A1)
  *   - GU_COLOR_4444 - 16-bit color (R4G4B4A4)
  *   - GU_COLOR_8888 - 32-bit color (R8G8B8A8)
  *
  *   - GU_NORMAL_8BIT - 8-bit normals
  *   - GU_NORMAL_16BIT - 16-bit normals
  *   - GU_NORMAL_32BITF - 32-bit normals (float)
  *
  *   - GU_VERTEX_8BIT - 8-bit vertex position
  *   - GU_VERTEX_16BIT - 16-bit vertex position
  *   - GU_VERTEX_32BITF - 32-bit vertex position (float)
  *
  *   - GU_WEIGHT_8BIT - 8-bit weights
  *   - GU_WEIGHT_16BIT - 16-bit weights
  *   - GU_WEIGHT_32BITF - 32-bit weights (float)
  *
  *   - GU_INDEX_8BIT - 8-bit vertex index
  *   - GU_INDEX_16BIT - 16-bit vertex index
  *
  *   - GU_WEIGHTS(n) - Number of weights (1-8)
  *   - GU_VERTICES(n) - Number of vertices (1-8)
  *
  *   - GU_TRANSFORM_2D - Coordinate is passed directly to the rasterizer
  *   - GU_TRANSFORM_3D - Coordinate is transformed before passed to rasterizer
  *
  * @note Every vertex must align to 32 bits, which means that you HAVE to pad if it does not add up!
  *
  * Vertex order:
  * [for vertices(1-8)]
  * [weights (0-8)]
  * [texture uv]
  * [color]
  * [normal]
  * [vertex]
  * [/for]
  *
  * @par Example: Render 400 triangles, with floating-point texture coordinates, and floating-point position, no indices
  * @code
  * sceGuDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_VERTEX_32BITF,400*3,0,vertices)
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param prim - What kind of primitives to render
  * @param vtype - Vertex type to process
  * @param count - How many vertices to process
  * @param indices - Optional pointer to an index-list
  * @param vertices - Pointer to a vertex-list
**/
//sceGumDrawArray(GU_TRIANGLE_STRIP,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,count,0,drawVTX);
struct VertexGUDA
{
	float u,v;
    unsigned int col;
    float x, y, z;
};
void sceGuDrawArray(int prim, int vtype, int count, const void* indices, const void* vertices)
{	
	verify(count>0 && indices==0 && vertices!=0 && prim==GU_TRIANGLE_STRIP && vtype==(GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D))
	VertexGUDA* vtx=(VertexGUDA*)vertices;
	//CRASHNOW
	glBegin(GL_TRIANGLE_STRIP);
	{
		while(count--)
		{
			glTexCoord2f(vtx->u,vtx->v);
			u8* ucol=(u8*)&vtx->col;
			//glColor4ubv((GLubyte*)&vtx->col);
			glColor4ub(ucol[0],ucol[1],ucol[2],ucol[3]);
			glVertex3f(vtx->x,vtx->y,vtx->z);
			vtx++;
		}
	}
	glEnd();

	
}

/**
  * Begin conditional rendering of object
  *
  * If no vertices passed into this function are inside the scissor region, it will skip rendering
  * the object. There can be up to 32 levels of conditional testing, and all levels HAVE to
  * be terminated by sceGuEndObject().
  *
  * @par Example: test a boundingbox against the frustum, and if visible, render object
  * @code
  * sceGuBeginObject(GU_VERTEX_32BITF,8,0,boundingBox)
{
	CRASHNOW
	
}
  *   sceGuDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_VERTEX_32BITF,vertexCount,0,vertices)
{
	CRASHNOW
	
}
  * sceGuEndObject()
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param vtype - Vertex type to process
  * @param count - Number of vertices to test
  * @param indices - Optional list to an index-list
  * @param vertices - Pointer to a vertex-list
**/
void sceGuBeginObject(int vtype, int count, const void* indices, const void* vertices)
{
	CRASHNOW
	
}

/**
  * End conditional rendering of object
**/
void sceGuEndObject(void)
{
	CRASHNOW
	
}

/**
  * Enable or disable GE state
  *
  * Look at sceGuEnable() for a list of states
  *
  * @param state - Which state to change
  * @param status - Wether to enable or disable the state
**/
void sceGuSetStatus(int state, int status)
{
	CRASHNOW
	
}

/**
  * Get if state is currently enabled or disabled
  *
  * Look at sceGuEnable() for a list of states
  *
  * @param state - Which state to query about
  * @returns Wether state is enabled or not
**/
int sceGuGetStatus(int state)
{
	CRASHNOW
	return 0;	
}

/**
  * Set the status on all 22 available states
  *
  * Look at sceGuEnable() for a list of states
  *
  * @param status - Bit-mask (0-21) containing the status of all 22 states
**/
void sceGuSetAllStatus(int status)
{
	CRASHNOW
	
}

/**
  * Query status on all 22 available states
  *
  * Look at sceGuEnable() for a list of states
  *
  * @returns Status of all 22 states as a bitmask (0-21)
**/
int sceGuGetAllStatus(void)
{
	CRASHNOW
	return 0;	
}

/**
  * Enable GE state
  *
  * The currently available states are:
		GU_ALPHA_TEST
		GU_DEPTH_TEST
		GU_SCISSOR_TEST
		GU_BLEND
		GU_CULL_FACE
		GU_DITHER
		GU_CLIP_PLANES
		GU_TEXTURE_2D
		GU_LIGHTING
		GU_LIGHT0
		GU_LIGHT1
		GU_LIGHT2
		GU_LIGHT3
		GU_LINE_SMOOTH	
		GU_PATCH_CULL_FACE	
		GU_COLOR_TEST		
		GU_COLOR_LOGIC_OP	
		GU_FACE_NORMAL_REVERSE
		GU_PATCH_FACE		
		GU_FRAGMENT_2X

  *
  * @param state - Which state to enable
**/
/*
#define GU_ALPHA_TEST		(0)
#define GU_DEPTH_TEST		(1)
#define GU_SCISSOR_TEST		(2)
#define GU_STENCIL_TEST		(3)
#define GU_BLEND		(4)
#define GU_CULL_FACE		(5)
#define GU_DITHER		(6)
#define GU_FOG			(7)
#define GU_CLIP_PLANES		(8)
#define GU_TEXTURE_2D		(9)
#define GU_LIGHTING		(10)
#define GU_LIGHT0		(11)
#define GU_LIGHT1		(12)
#define GU_LIGHT2		(13)
#define GU_LIGHT3		(14)
#define GU_LINE_SMOOTH		(15)
#define GU_PATCH_CULL_FACE	(16)
#define GU_COLOR_TEST		(17)
#define GU_COLOR_LOGIC_OP	(18)
#define GU_FACE_NORMAL_REVERSE	(19)
#define GU_PATCH_FACE		(20)
#define GU_FRAGMENT_2X		(21)
*/
const GLint gu2gl[] = 
{
	GL_ALPHA_TEST,//GU_ALPHA_TEST
	GL_DEPTH_TEST,//GU_DEPTH_TEST
	GL_SCISSOR_TEST,//GU_SCISSOR_TEST
	GL_STENCIL_TEST,//GU_STENCIL_TEST
	GL_BLEND,//GU_BLEND
	GL_CULL_FACE,//GU_CULL_FACE
	GL_DITHER,//GU_DITHER
	GL_FOG,//GU_FOG
	GL_CLIP_PLANE5,//GU_CLIP_PLANES
	GL_TEXTURE_2D,//GU_TEXTURE_2D
	GL_LIGHTING,//GU_LIGHTING
	GL_LIGHT0,//GU_LIGHT0
	GL_LIGHT1,//GU_LIGHT1
	GL_LIGHT2,//GU_LIGHT2
	GL_LIGHT3,//GU_LIGHT3
	GL_LINE_SMOOTH,//GU_LINE_SMOOTH	
	-1,//GU_PATCH_CULL_FACE	
	-1,//GU_COLOR_TEST		
	-1,//GU_COLOR_LOGIC_OP	
	-1,//GU_FACE_NORMAL_REVERSE
	-1,//GU_PATCH_FACE		
	-1,//GU_FRAGMENT_2X
};
void sceGuEnable(int state)
{
	verify(gu2gl[state]!=-1);
	glEnable(gu2gl[state]);
	//CRASHNOW
	//GU_ALPHA_TEST
}

/**
  * Disable GE state
  *
  * Look at sceGuEnable() for a list of states
  *
  * @param state - Which state to disable
**/
void sceGuDisable(int state)
{
	verify(gu2gl[state]!=-1);
	glDisable(gu2gl[state]);
	//CRASHNOW
	
}

/**
  * Set light parameters
  *
  * Available light types are:
  *   - GU_DIRECTIONAL - Directional light
  *   - GU_POINTLIGHT - Single point of light
  *   - GU_SPOTLIGHT - Point-light with a cone
  *
  * Available light components are:
  *   - GU_AMBIENT_AND_DIFFUSE
  *   - GU_DIFFUSE_AND_SPECULAR
  *   - GU_UNKNOWN_LIGHT_COMPONENT
  *
  * @param light - Light index
  * @param type - Light type
  * @param components - Light components
  * @param position - Light position
**/
void sceGuLight(int light, int type, int components, const ScePspFVector3* position)
{
	CRASHNOW
	
}

/**
  * Set light attenuation
  *
  * @param light - Light index
  * @param atten0 - Constant attenuation factor
  * @param atten1 - Linear attenuation factor
  * @param atten2 - Quadratic attenuation factor
**/
void sceGuLightAtt(int light, float atten0, float atten1, float atten2)
{
	CRASHNOW
	
}

/**
  * Set light color
  *
  * Available light components are:
  *   - GU_AMBIENT
  *   - GU_DIFFUSE
  *   - GU_SPECULAR
  *   - GU_AMBIENT_AND_DIFFUSE
  *   - GU_DIFFUSE_AND_SPECULAR
  *
  * @param light - Light index
  * @param component - Which component to set
  * @param color - Which color to use
**/
void sceGuLightColor(int light, int component, unsigned int color)
{
	CRASHNOW
	
}

/**
  * Set light mode
  *
  * Available light modes are:
  *   - GU_SINGLE_COLOR
  *   - GU_SEPARATE_SPECULAR_COLOR
  *
  * Separate specular colors are used to interpolate the specular component
  * independently, so that it can be added to the fragment after the texture color.
  *
  * @param mode - Light mode to use
**/
void sceGuLightMode(int mode)
{
	CRASHNOW
	
}

/**
  * Set spotlight parameters
  *
  * @param light - Light index
  * @param direction - Spotlight direction
  * @param exponent - Spotlight exponent
  * @param cutoff - Spotlight cutoff angle (in radians)
**/
void sceGuLightSpot(int light, const ScePspFVector3* direction, float exponent, float cutoff)
{
	CRASHNOW
	
}

/**
  * Clear current drawbuffer
  *
  * Available clear-flags are (OR them together to get final clear-mode):
  *   - GU_COLOR_BUFFER_BIT - Clears the color-buffer
  *   - GU_STENCIL_BUFFER_BIT - Clears the stencil-buffer
  *   - GU_DEPTH_BUFFER_BIT - Clears the depth-buffer
  *
  * @param flags - Which part of the buffer to clear
**/
void sceGuClear(int flags)
{
	GLint gm=0;
	if (flags&GU_COLOR_BUFFER_BIT) gm|=GL_COLOR_BUFFER_BIT;
	if (flags&GU_STENCIL_BUFFER_BIT) gm|=GL_STENCIL_BUFFER_BIT;
	if (flags&GU_DEPTH_BUFFER_BIT) gm|=GL_DEPTH_BUFFER_BIT;
	
	glClear(gm);
	//CRASHNOW
	
}

/**
  * Set the current clear-color
  *
  * @param color - Color to clear with
**/
void sceGuClearColor(unsigned int color)
{
	//CRASHNOW	
	u8* arr=(u8*)&color;
 	glClearColor(arr[0]/255.f,arr[1]/255.f,arr[2]/255.f,arr[3]/255.f);
}

/**
  * Set the current clear-depth
  *
  * @param depth - Set which depth to clear with (0x0000-0xffff)
**/
void sceGuClearDepth(unsigned int depth)
{
	//CRASHNOW
	glClearDepth(depth/65535.f);
	
}

/**
  * Set the current stencil clear value
  *
  * @param stencil - Set which stencil value to clear with (0-255)
  *
**/
void sceGuClearStencil(unsigned int stencil)
{
	//CRASHNOW
	glClearStencil(stencil);
	
}

/**
  * Set mask for which bits of the pixels to write
  *
  * @param mask - Which bits to filter against writes
  *
**/
void sceGuPixelMask(unsigned int mask)
{
	CRASHNOW
	
}

/**
  * Set current primitive color
  *
  * @param color - Which color to use (overriden by vertex-colors)
**/
void sceGuColor(unsigned int color)
{
	CRASHNOW
	
}

/**
  * Set the color test function
  *
  * The color test is only performed while GU_COLOR_TEST is enabled.
  *
  * Available functions are:
  *   - GU_NEVER
  *   - GU_ALWAYS
  *   - GU_EQUAL
  *   - GU_NOTEQUAL
  *
  * @par Example: Reject any pixel that does not have 0 as the blue channel
  * @code
  * sceGuColorFunc(GU_EQUAL,0,0xff0000)
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param func - Color test function
  * @param color - Color to test against
  * @param mask - Mask ANDed against both source and destination when testing
**/
void sceGuColorFunc(int func, unsigned int color, unsigned int mask)
{
	CRASHNOW
	
}

/**
  * Set which color components that the material will receive
  *
  * The components are ORed together from the following values:
  *   - GU_AMBIENT
  *   - GU_DIFFUSE
  *   - GU_SPECULAR
  *
  * @param components - Which components to receive
**/
void sceGuColorMaterial(int components)
{
	CRASHNOW
	
}

/**
  * Set the alpha test parameters
  * 
  * Available comparison functions are:
  *   - GU_NEVER
  *   - GU_ALWAYS
  *   - GU_EQUAL
  *   - GU_NOTEQUAL
  *   - GU_LESS
  *   - GU_LEQUAL
  *   - GU_GREATER
  *   - GU_GEQUAL
  *
  * @param func - Specifies the alpha comparison function.
  * @param value - Specifies the reference value that incoming alpha values are compared to.
  * @param mask - Specifies the mask that both values are ANDed with before comparison.
**/
void sceGuAlphaFunc(int func, int value, int mask)
{
	//CRASHNOW
	verify(mask==255);
	glAlphaFunc(gu2gl_df[func],value/255.0f);
	
}

void sceGuAmbient(unsigned int color)
{
	CRASHNOW
	
}
void sceGuAmbientColor(unsigned int color)
{
	CRASHNOW
	
}

/**
  * Set the blending-mode
  *
  * Keys for the blending operations:
  *   - Cs - Source color
  *   - Cd - Destination color
  *   - Bs - Blend function for source fragment
  *   - Bd - Blend function for destination fragment
  *
  * Available blending-operations are:
  *   - GU_ADD - (Cs*Bs) + (Cd*Bd)
  *   - GU_SUBTRACT - (Cs*Bs) - (Cd*Bd)
  *   - GU_REVERSE_SUBTRACT - (Cd*Bd) - (Cs*Bs)
  *   - GU_MIN - Cs < Cd ? Cs : Cd
  *   - GU_MAX - Cs < Cd ? Cd : Cs
  *   - GU_ABS - |Cs-Cd|
  *
  * Available blending-functions are:
  *   - GU_SRC_COLOR
  *   - GU_ONE_MINUS_SRC_COLOR
  *   - GU_SRC_ALPHA
  *   - GU_ONE_MINUS_SRC_ALPHA
  *   - GU_DST_ALPHA
  *   - GU_ONE_MINUS_DST_ALPHA
  *   - GU_DST_COLOR
  *   - GU_ONE_MINUS_DST_COLOR
  *   - GU_FIX
  *
  * @param op - Blending Operation
  * @param src - Blending function for source operand
  * @param dest - Blending function for dest operand
  * @param srcfix - Fix value for GU_FIX (source operand)
  * @param destfix - Fix value for GU_FIX (dest operand)
**/
GLint gu2gl_blend[]=
{
	GL_SRC_COLOR,//GU_SRC_COLOR
	GL_ONE_MINUS_SRC_COLOR,//GU_ONE_MINUS_SRC_COLOR
	GL_SRC_ALPHA,//GU_SRC_ALPHA
	GL_ONE_MINUS_SRC_ALPHA,//GU_ONE_MINUS_SRC_ALPHA
	GL_DST_ALPHA,//GU_DST_ALPHA
	GL_ONE_MINUS_DST_ALPHA,//GU_ONE_MINUS_DST_ALPHA
	GL_DST_COLOR,//GU_DST_COLOR
	GL_ONE_MINUS_DST_COLOR,//GU_ONE_MINUS_DST_COLOR
	GL_ONE,//GU_FIX
};
void sceGuBlendFunc(int op, int src, int dest, unsigned int srcfix, unsigned int destfix)
{
	verify(GU_ADD==op);
	verify(gu2gl_blend[src]!=-1);
	verify(gu2gl_blend[dest]!=-1);
	glBlendFunc(gu2gl_blend[src],gu2gl_blend[dest]);
	//CRASHNOW
	
}

void sceGuMaterial(int mode, int color)
{
	CRASHNOW
	
}

/**
  *
**/
void sceGuModelColor(unsigned int emissive, unsigned int ambient, unsigned int diffuse, unsigned int specular)
{
	CRASHNOW
	
}

/**
  * Set stencil function and reference value for stencil testing
  *
  * Available functions are:
  *   - GU_NEVER
  *   - GU_ALWAYS
  *   - GU_EQUAL
  *   - GU_NOTEQUAL
  *   - GU_LESS
  *   - GU_LEQUAL
  *   - GU_GREATER
  *   - GU_GEQUAL
  *
  * @param func - Test function
  * @param ref - The reference value for the stencil test
  * @param mask - Mask that is ANDed with both the reference value and stored stencil value when the test is done
**/
void sceGuStencilFunc(int func, int ref, int mask)
{
	CRASHNOW
	
}

/**
  * Set the stencil test actions
  *
  * Available actions are:
  *   - GU_KEEP - Keeps the current value
  *   - GU_ZERO - Sets the stencil buffer value to zero
  *   - GU_REPLACE - Sets the stencil buffer value to ref, as specified by sceGuStencilFunc()
  *   - GU_INCR - Increments the current stencil buffer value
  *   - GU_DECR - Decrease the current stencil buffer value
  *   - GU_INVERT - Bitwise invert the current stencil buffer value
  *
  * As stencil buffer shares memory with framebuffer alpha, resolution of the buffer
  * is directly in relation.
  *
  * @param fail - The action to take when the stencil test fails
  * @param zfail - The action to take when stencil test passes, but the depth test fails
  * @param zpass - The action to take when both stencil test and depth test passes
**/
void sceGuStencilOp(int fail, int zfail, int zpass)
{
	CRASHNOW
	
}

/**
  * Set the specular power for the material
  *
  * @param power - Specular power
  *
**/
void sceGuSpecular(float power)
{
	CRASHNOW
	
}

/**
  * Set the current face-order (for culling)
  *
  * This only has effect when culling is enabled (GU_CULL_FACE)
  *
  * Culling order can be:
  *   - GU_CW - Clockwise primitives are not culled
  *   - GU_CCW - Counter-clockwise are not culled
  *
  * @param order - Which order to use
**/
GLint gu2gl_ff[]
=
{
	GL_CW,//GU_CW
	GL_CCW,//GU_CCW
};
void sceGuFrontFace(int order)
{
	//CRASHNOW
	glFrontFace(gu2gl_ff[order]);
}

/**
  * Set color logical operation
  *
  * Available operations are:
  *   - GU_CLEAR
  *   - GU_AND
  *   - GU_AND_REVERSE 
  *   - GU_COPY
  *   - GU_AND_INVERTED
  *   - GU_NOOP
  *   - GU_XOR
  *   - GU_OR
  *   - GU_NOR
  *   - GU_EQUIV
  *   - GU_INVERTED
  *   - GU_OR_REVERSE
  *   - GU_COPY_INVERTED
  *   - GU_OR_INVERTED
  *   - GU_NAND
  *   - GU_SET
  *
  * This operation only has effect if GU_COLOR_LOGIC_OP is enabled.
  *
  * @param op - Operation to execute
**/
void sceGuLogicalOp(int op)
{
	CRASHNOW
	
}

/**
  * Set ordered pixel dither matrix
  *
  * This dither matrix is only applied if GU_DITHER is enabled.
  *
  * @param matrix - Dither matrix
**/
void sceGuSetDither(const ScePspIMatrix4* matrix)
{
	CRASHNOW
	
}

/**
  * Set how primitives are shaded
  *
  * The available shading-methods are:
  *   - GU_FLAT - Primitives are flatshaded, the last vertex-color takes effet
  *   - GU_SMOOTH - Primtives are gouraud-shaded, all vertex-colors take effect
  *
  * @param mode - Which mode to use
**/
GLint gu2gl_sm[] =
{
	GL_FLAT,//GU_FLAT
	GL_SMOOTH,//GU_SMOOTH
};
void sceGuShadeModel(int mode)
{
	//CRASHNOW
	glShadeModel(gu2gl_sm[mode]);
	
}

/**
  * Image transfer using the GE
  *
  * @note Data must be aligned to 1 quad word (16 bytes)
  *
  * @par Example: Copy a fullscreen 32-bit image from RAM to VRAM
  * @code
  * sceGuCopyImage(GU_PSM_8888,0,0,480,272,512,pixels,0,0,512,(void*)(((unsigned int)framebuffer)+0x4000000))
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param psm - Pixel format for buffer
  * @param sx - Source X
  * @param sy - Source Y
  * @param width - Image width
  * @param height - Image height
  * @param srcw - Source buffer width (block aligned)
  * @param src - Source pointer
  * @param dx - Destination X
  * @param dy - Destination Y
  * @param destw - Destination buffer width (block aligned)
  * @param dest - Destination pointer
**/
void sceGuCopyImage(int psm, int sx, int sy, int width, int height, int srcw, void* src, int dx, int dy, int destw, void* dest)
{
	CRASHNOW
	
}

/**
  * Specify the texture environment color
  *
  * This is used in the texture function when a constant color is needed.
  *
  * See sceGuTexFunc() for more information.
  *
  * @param color - Constant color (0x00BBGGRR)
**/
void sceGuTexEnvColor(unsigned int color)
{
	CRASHNOW
	
}

/**
  * Set how the texture is filtered
  *
  * Available filters are:
  *   - GU_NEAREST
  *   - GU_LINEAR
  *   - GU_NEAREST_MIPMAP_NEAREST
  *   - GU_LINEAR_MIPMAP_NEAREST
  *   - GU_NEAREST_MIPMAP_LINEAR
  *   - GU_LINEAR_MIPMAP_LINEAR
  *
  * @param min - Minimizing filter
  * @param mag - Magnifying filter
**/
GLint gu2gl_gtf[]=
{
	GL_NEAREST,//GU_NEAREST
	GL_LINEAR,//GU_LINEAR
	GL_NEAREST_MIPMAP_NEAREST,//GU_NEAREST_MIPMAP_NEAREST
	GL_LINEAR_MIPMAP_NEAREST,//GU_LINEAR_MIPMAP_NEAREST
	GL_NEAREST_MIPMAP_LINEAR,//GU_NEAREST_MIPMAP_LINEAR
	GL_LINEAR_MIPMAP_LINEAR,//GU_LINEAR_MIPMAP_LINEAR
};
void sceGuTexFilter(int min, int mag)
{
	//CRASHNOW
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,gu2gl_gtf[min] );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,gu2gl_gtf[mag] );
}

/**
  * Flush texture page-cache
  *
  * Do this if you have copied/rendered into an area currently in the texture-cache
  *
**/
void sceGuTexFlush(void)
{
	CRASHNOW
	
}

/**
  * Set how textures are applied
  *
  * Key for the apply-modes:
  *   - Cv - Color value result
  *   - Ct - Texture color
  *   - Cf - Fragment color
  *   - Cc - Constant color (specified by sceGuTexEnvColor())
  *
  * Available apply-modes are: (TFX)
  *   - GU_TFX_MODULATE - Cv=Ct*Cf TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
  *   - GU_TFX_DECAL - TCC_RGB: Cv=Ct,Av=Af TCC_RGBA: Cv=Cf*(1-At)+Ct*At Av=Af
  *   - GU_TFX_BLEND - Cv=(Cf*(1-Ct))+(Cc*Ct) TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
  *   - GU_TFX_REPLACE - Cv=Ct TCC_RGB: Av=Af TCC_RGBA: Av=At
  *   - GU_TFX_ADD - Cv=Cf+Ct TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
  *
  * The fields TCC_RGB and TCC_RGBA specify components that differ between
  * the two different component modes.
  *
  *   - GU_TFX_MODULATE - The texture is multiplied with the current diffuse fragment
  *   - GU_TFX_REPLACE - The texture replaces the fragment
  *   - GU_TFX_ADD - The texture is added on-top of the diffuse fragment
  *   
  * Available component-modes are: (TCC)
  *   - GU_TCC_RGB - The texture alpha does not have any effect
  *   - GU_TCC_RGBA - The texture alpha is taken into account
  *
  * @param tfx - Which apply-mode to use
  * @param tcc - Which component-mode to use
**/
GLint gu2gl_tfx[]=
{
	GL_MODULATE,//GU_TFX_MODULATE - Cv=Ct*Cf TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
	GL_DECAL,//GU_TFX_DECAL - TCC_RGB: Cv=Ct,Av=Af TCC_RGBA: Cv=Cf*(1-At)+Ct*At Av=Af
	GL_BLEND,//GU_TFX_BLEND - Cv=(Cf*(1-Ct))+(Cc*Ct) TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
	GL_REPLACE,//GU_TFX_REPLACE - Cv=Ct TCC_RGB: Av=Af TCC_RGBA: Av=At
	-1,//GU_TFX_ADD - Cv=Cf+Ct TCC_RGB: Av=Af TCC_RGBA: Av=At*Af
};
void sceGuTexFunc(int tfx, int tcc)
{
	verify(gu2gl_tfx[tfx]!=-1 && tcc==GU_TCC_RGBA);
	//CRASHNOW
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, gu2gl_tfx[tfx] ) ;
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


}

/**
  * Set current texturemap
  *
  * Textures may reside in main RAM, but it has a huge speed-penalty. Swizzle textures
  * to get maximum speed.
  *
  * @note Data must be aligned to 1 quad word (16 bytes)
  *
  * @param mipmap - Mipmap level
  * @param width - Width of texture (must be a power of 2)
  * @param height - Height of texture (must be a power of 2)
  * @param tbw - Texture Buffer Width (block-aligned)
  * @param tbp - Texture buffer pointer (16 byte aligned)
**/
int TexModeFMT;
int TexModeFMT_PAL;
u16* colors;
GLuint gltexture=-1;
/*
#define GU_PSM_5650		(0) // * Display, Texture, Palette *
#define GU_PSM_5551		(1) // * Display, Texture, Palette *
#define GU_PSM_4444		(2) // * Display, Texture, Palette *
#define GU_PSM_8888		(3) // * Display, Texture, Palette *
#define GU_PSM_T4		(4) // * Texture *
#define GU_PSM_T8		(5) // * Texture *
#define GU_PSM_T16		(6) // * Texture *
#define GU_PSM_T32		(7) // * Texture *
#define GU_PSM_DXT1		(8) // * Texture *
#define GU_PSM_DXT3		(9) // * Texture *
#define GU_PSM_DXT5	   (10) // * Texture *
*/

/*
//Yeah, its a dream working with opengl
#define GL_BGR					0x80E0
#define GL_BGRA					0x80E1
#define GL_UNSIGNED_BYTE_3_3_2			0x8032
#define GL_UNSIGNED_BYTE_2_3_3_REV		0x8362
#define GL_UNSIGNED_SHORT_5_6_5			0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV		0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4		0x8033
#define GL_UNSIGNED_SHORT_4_4_4_4_REV		0x8365
#define GL_UNSIGNED_SHORT_5_5_5_1		0x8034
#define GL_UNSIGNED_SHORT_1_5_5_5_REV		0x8366
#define GL_UNSIGNED_INT_8_8_8_8			0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV		0x8367
#define GL_UNSIGNED_INT_10_10_10_2		0x8036
#define GL_UNSIGNED_INT_2_10_10_10_REV		0x8368
*/
#ifndef GLEW_OK
#define GL_UNSIGNED_SHORT_5_6_5_REV GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT_1_5_5_5_REV GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT_4_4_4_4_REV GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT_4_4_4_4 GL_UNSIGNED_SHORT
#define GL_UNSIGNED_INT_8_8_8_8_REV GL_UNSIGNED_SHORT
#define GL_UNSIGNED_INT_8_8_8_8 GL_UNSIGNED_SHORT
#define GL_UNSIGNED_BYTE_3_3_2 GL_UNSIGNED_BYTE

#endif
GLuint gutf2gl[11][3]=
{
	{ GL_RGB,  GL_UNSIGNED_SHORT_5_6_5_REV ,0},	//GU_PSM_5650
	{ GL_RGBA,  GL_UNSIGNED_SHORT_1_5_5_5_REV ,0},	//GU_PSM_5551
	{ GL_RGBA,  GL_UNSIGNED_SHORT_4_4_4_4_REV ,0},	//GU_PSM_4444
	{ GL_RGBA,  GL_UNSIGNED_INT_8_8_8_8_REV ,0},	//GU_PSM_8888
	{ GL_RGB,  GL_UNSIGNED_BYTE_3_3_2 ,0},	//GU_PSM_T4
	{ GL_RGB,  GL_UNSIGNED_BYTE ,-1},	//GU_PSM_T8
	{ GL_RGBA,  GL_UNSIGNED_SHORT_4_4_4_4 ,0},	//GU_PSM_T16
	{ GL_RGBA,  GL_UNSIGNED_INT_8_8_8_8 ,0},	//GU_PSM_T32
	{ GL_RGB,  GL_UNSIGNED_BYTE_3_3_2 ,0},	//GU_PSM_DXT1
	{ GL_RGB,  GL_UNSIGNED_BYTE_3_3_2 ,0},	//GU_PSM_DXT3
	{ GL_RGB,  GL_UNSIGNED_BYTE_3_3_2 ,0},	//GU_PSM_DXT5
};


/**
  * Set texture-mode parameters
  *
  * Available texture-formats are:
  *   - GU_PSM_5650 - Hicolor, 16-bit
  *   - GU_PSM_5551 - Hicolor, 16-bit
  *   - GU_PSM_4444 - Hicolor, 16-bit
  *   - GU_PSM_8888 - Truecolor, 32-bit
  *   - GU_PSM_T4 - Indexed, 4-bit (2 pixels per byte)
  *   - GU_PSM_T8 - Indexed, 8-bit
  *
  * @param tpsm - Which texture format to use
  * @param maxmips - Number of mipmaps to use (0-8)
  * @param a2 - Unknown, set to 0
  * @param swizzle - GU_TRUE(1) to swizzle texture-reads
**/
void sceGuTexMode(int tpsm, int maxmips, int a2, int swizzle)
{
	TexModeFMT=tpsm;
	verify(swizzle==0);
	//CRASHNOW
	
}

/**
  * Upload CLUT (Color Lookup Table)
  *
  * @note Data must be aligned to 1 quad word (16 bytes)
  *
  * @param num_blocks - How many blocks of 8 entries to upload (32*8 is 256 colors)
  * @param cbp - Pointer to palette (16 byte aligned)
**/
void sceGuClutLoad(int num_blocks, const void* cbp)
{
	//CRASHNOW
	verify(num_blocks==32);
	//TexModeFMT_PAL
	//glColorTable(
	// (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
	//glColorTable(GL_COLOR_TABLE, gutf2gl[TexModeFMT_PAL][0], num_blocks*32, gutf2gl[TexModeFMT_PAL][1], GL_UNSIGNED_BYTE, cbp);
	colors=(u16*)cbp;
}

/**
  * Set current CLUT mode
  *
  * Available pixel formats for palettes are:
  *   - GU_PSM_5650
  *   - GU_PSM_5551
  *   - GU_PSM_4444
  *   - GU_PSM_8888
  *
  * @param cpsm - Which pixel format to use for the palette
  * @param shift - Shifts color index by that many bits to the right
  * @param mask - Masks the color index with this bitmask after the shift (0-0xFF)
  * @param a3 - Unknown, set to 0
**/
void sceGuClutMode(unsigned int cpsm, unsigned int shift, unsigned int mask, unsigned int a3)
{
	//CRASHNOW
	verify(shift==0 && mask==255);
	verify(cpsm<=3);
	TexModeFMT_PAL=cpsm;
}
u16 temp_text[1024*1024];

void sceGuTexImage(int mipmap, int width, int height, int tbw, const void* tbp)
{
	//verify(width==tbw);
	width=tbw;
	if (gltexture==-1)
	{
		glGenTextures(1,&gltexture);
		glBindTexture(GL_TEXTURE_2D,gltexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  
	}
	if (gutf2gl[TexModeFMT][2]==-1)
	{
		int sz=width*height;
		u16* dst=temp_text;
		u8* src=(u8*)tbp;
		if (colors!=0)
		{
			while(sz--)
				*dst++=colors[*src++];
		}
		glTexImage2D(GL_TEXTURE_2D,mipmap,gutf2gl[TexModeFMT_PAL][0]/*internal ?*/,width,height,0,gutf2gl[TexModeFMT_PAL][0],gutf2gl[TexModeFMT_PAL][1],temp_text);	
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D,mipmap,gutf2gl[TexModeFMT][0]/*internal ?*/,width,height,0,gutf2gl[TexModeFMT][0],gutf2gl[TexModeFMT][1],tbp);
	}
	//CRASHNOW
	
}

/**
  * Set texture-level mode (mipmapping)
  *
  * Available modes are:
  *   - GU_TEXTURE_AUTO
  *   - GU_TEXTURE_CONST
  *   - GU_TEXTURE_SLOPE
  *
  * @param mode - Which mode to use
  * @param bias - Which mipmap bias to use
**/
void sceGuTexLevelMode(unsigned int mode, float bias)
{
	CRASHNOW
	
}

/**
  * Set the texture-mapping mode
  *
  * Available modes are:
  *   - GU_TEXTURE_COORDS
  *   - GU_TEXTURE_MATRIX
  *   - GU_ENVIRONMENT_MAP
  *
  * @param mode - Which mode to use
  * @param a1 - Unknown
  * @param a2 - Unknown
**/
void sceGuTexMapMode(int mode, unsigned int a1, unsigned int a2)
{
	CRASHNOW
	
}

/**
  * Set texture offset
  *
  * @note Only used by the 3D T&L pipe, renders done with GU_TRANSFORM_2D are
  * not affected by this.
  *
  * @param u - Offset to add to the U coordinate
  * @param v - Offset to add to the V coordinate
**/
void sceGuTexOffset(float u, float v)
{
	verify(u==0 && v==0);
	//CRASHNOW
	
}

/**
  * Set texture projection-map mode
  *
  * Available modes are:
  *   - GU_POSITION
  *   - GU_UV
  *   - GU_NORMALIZED_NORMAL
  *   - GU_NORMAL
  *
  * @param mode - Which mode to use
**/
void sceGuTexProjMapMode(int mode)
{
	CRASHNOW
	
}

/**
  * Set texture scale
  *
  * @note Only used by the 3D T&L pipe, renders ton with GU_TRANSFORM_2D are
  * not affected by this.
  *
  * @param u - Scalar to multiply U coordinate with
  * @param v - Scalar to multiply V coordinate with
**/
void sceGuTexScale(float u, float v)
{
	verify(u==1 && v==1);
	//CRASHNOW
	
}
void sceGuTexSlope(float slope)
{
	CRASHNOW
	
}

/**
  * Synchronize rendering pipeline with image upload.
  *
  * This will stall the rendering pipeline until the current image upload initiated by
  * sceGuCopyImage() has completed.
**/
void sceGuTexSync()
{
	CRASHNOW
	
}

/**
  * Set if the texture should repeat or clamp
  *
  * Available modes are:
  *   - GU_REPEAT - The texture repeats after crossing the border
  *   - GU_CLAMP - Texture clamps at the border
  *
  * @param u - Wrap-mode for the U direction
  * @param v - Wrap-mode for the V direction
**/
void sceGuTexWrap(int u, int v)
{
	CRASHNOW
	
}


/**
  * Set virtual coordinate offset
  *
  * The PSP has a virtual coordinate-space of 4096x4096, this controls where rendering is performed
  * 
  * @par Example: Center the virtual coordinate range
  * @code
  * sceGuOffset(2048-(480/2),2048-(480/2))
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param x - Offset (0-4095)
  * @param y - Offset (0-4095)
**/
void sceGuOffset(unsigned int x, unsigned int y)
{
	verify(x==2048-(480/2) && y==2048-(272/2));
	//CRASHNOW
	
}

/**
  * Set what to scissor within the current viewport
  *
  * Note that scissoring is only performed if the custom scissoring is enabled (GU_SCISSOR_TEST)
  *
  * @param x - Left of scissor region
  * @param y - Top of scissor region
  * @param w - Width of scissor region
  * @param h - Height of scissor region
**/
void sceGuScissor(int x, int y, int w, int h)
{
	//glScissor(x,y,w,h);
	//CRASHNOW
	
}

/**
  * Set current viewport
  *
  * @par Example: Setup a viewport of size (480,272) with origo at (2048,2048)
  * @code
  * sceGuViewport(2048,2048,480,272)
{
	CRASHNOW
	
}
  * @endcode
  *
  * @param cx - Center for horizontal viewport
  * @param cy - Center for vertical viewport
  * @param width - Width of viewport
  * @param height - Height of viewport
**/
void sceGuViewport(int cx, int cy, int width, int height)
{
	//glViewport
	//CRASHNOW
	verify(cx==2048 && cy==2048);
	//glViewport(cx-width/2,cy-height/2,width,height);
	//glViewport(-width/2,-height/2,width,height);
	//glOffset
	glViewport(0, 0, 480,272);
	//glPolygonMode(GL_BACK,GL_LINE);
	//glPolygonMode(GL_FRONT,GL_LINE);
	
}

/**
  * Draw bezier surface
  *
  * @param vtype - Vertex type, look at sceGuDrawArray() for vertex definition
  * @param ucount - Number of vertices used in the U direction
  * @param vcount - Number of vertices used in the V direction
  * @param indices - Pointer to index buffer
  * @param vertices - Pointer to vertex buffer
**/
void sceGuDrawBezier(int vtype, int ucount, int vcount, const void* indices, const void* vertices)
{
	CRASHNOW
	
}

/**
  * Set dividing for patches (beziers and splines)
  *
  * @param ulevel - Number of division on u direction
  * @param vlevel - Number of division on v direction
**/
void sceGuPatchDivide(unsigned int ulevel, unsigned int vlevel)
{
	CRASHNOW
	
}

void sceGuPatchFrontFace(unsigned int a0)
{
	CRASHNOW
	
}

/**
  * Set primitive for patches (beziers and splines)
  *
  * @param prim - Desired primitive type (GU_POINTS | GU_LINE_STRIP | GU_TRIANGLE_STRIP)
**/
void sceGuPatchPrim(int prim)
{
	CRASHNOW
	
}

void sceGuDrawSpline(int vtype, int ucount, int vcount, int uedge, int vedge, const void* indices, const void* vertices)
{
	CRASHNOW
	
}

/**
  * Set transform matrices
  *
  * Available matrices are:
  *   - GU_PROJECTION - View->Projection matrix
  *   - GU_VIEW - World->View matrix
  *   - GU_MODEL - Model->World matrix
  *   - GU_TEXTURE - Texture matrix
  *
  * @param type - Which matrix-type to set
  * @param matrix - Matrix to load
**/
GLuint gu2gl_smtx[]=
{
	GL_PROJECTION,
	-1,
	GL_MODELVIEW,
	GL_TEXTURE,
};
void sceGuSetMatrix(int type, const ScePspFMatrix4* matrix)
{
	//CRASHNOW
	/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();*/
/*
	glMatrixMode(GU_MODEL);
	glLoadIdentity();*/
	
	if (type==1)
		return;
	verify(gu2gl_smtx[type]!=-1);
	glMatrixMode(gu2gl_smtx[type]);
	glLoadIdentity();
	glMultMatrixf((float*)matrix);
}

/**
  * Specify skinning matrix entry
  *
  * To enable vertex skinning, pass GU_WEIGHTS(n), where n is between
  * 1-8, and pass available GU_WEIGHT_??? declaration. This will change
  * the amount of weights passed in the vertex araay, and by setting the skinning,
  * matrices, you will multiply each vertex every weight and vertex passed.
  *
  * Please see sceGuDrawArray() for vertex format information.
  *
  * @param index - Skinning matrix index (0-7)
  * @param matrix - Matrix to set
**/
void sceGuBoneMatrix(unsigned int index, const ScePspFMatrix4* matrix)
{
	CRASHNOW
	
}

/**
  * Specify morph weight entry
  *
  * To enable vertex morphing, pass GU_VERTICES(n), where n is between
  * 1-8. This will change the amount of vertices passed in the vertex array,
  * and by setting the morph weights for every vertex entry in the array,
  * you can blend between them.
  *
  * Please see sceGuDrawArray() for vertex format information.
  *
  * @param index - Morph weight index (0-7)
  * @param weight - Weight to set
**/
void sceGuMorphWeight(int index, float weight)
{
	CRASHNOW
	
}

void sceGuDrawArrayN(int primitive_type, int vertex_type, int count, int a3, const void* indices, const void* vertices)
{
	CRASHNOW
	
}

/**
  * Set how the display should be set
  *
  * Available behaviours are:
  *   - PSP_DISPLAY_SETBUF_IMMEDIATE - Display is swapped immediately
  *   - PSP_DISPLAY_SETBUF_NEXTFRAME - Display is swapped on the next frame
  *
  * Do remember that this swaps the pointers internally, regardless of setting, so be careful to wait until the next
  * vertical blank or use another buffering algorithm (see guSwapBuffersCallback()).
**/
void guSwapBuffersBehaviour(int behaviour)
{
	CRASHNOW
	
}

/**
  * Set a buffer swap callback to allow for more advanced buffer methods without hacking the library.
  *
  * The GuSwapBuffersCallback is defined like this:
  * @code
  * void swapBuffersCallback(void** display, void** render)
{
	CRASHNOW
	
}
  * @endcode
  * and on entry they contain the variables that are to be set. To change the pointers that will be used, just
  * write the new pointers. Example of a triple-buffering algorithm:
  * @code
  * void* doneBuffer;
  * void swapBuffersCallback(void** display, void** render)
  * {
  *  void* active = doneBuffer;
  *  doneBuffer = *display;
     *display = active;
  * }
  * @endcode
  *
  * @param callback - Callback to access when buffers are swapped. Pass 0 to disable.
**/
void guSwapBuffersCallback(GuSwapBuffersCallback callback)
{
	CRASHNOW
	
}

/*@}*/
u8* vram_buffers;
void* getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm)
{
	return vram_buffers++;	
}
void* getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm)
{
	return vram_buffers++;	
}

#endif