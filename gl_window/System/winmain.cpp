// Includes

#include <windows.h>
// For command line processing
#include <vector>
#include <algorithm>
//
#include "framework.h"
#include "rawinput.h"
#include "dwmapi.h"
// Included just for testing.
#include "sys_gl.h"
#include "gl_prim_debug.h"
#include "emulator.h"
#include "ini.h"
#include "wintimer.h"

//Globals
HWND hWnd;
int SCREEN_W = 1024;
int SCREEN_H = 768;

#pragma comment (lib,"dwmapi.lib")
#pragma comment(lib, "winmm.lib")

// Function Declarations
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int KeyCheck(int keynum)
{
	int i;
	static int hasrun = 0;
	static int keys[256];
	//Init
	if (hasrun == 0) { for (i = 0; i < 256; i++) { keys[i] = 0; }	hasrun = 1; }

	if (!keys[keynum] && key[keynum]) //Return True if not in que
	{
		keys[keynum] = 1;	return 1;
	}
	else if (keys[keynum] && !key[keynum]) //Return False if in que
		keys[keynum] = 0;
	return 0;
}

void osMessage(int ID, const char* fmt, ...)
{
	int mType = MB_ICONERROR;
	char		text[256] = "";								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf_s(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);

	switch (ID)
	{
	case IDCANCEL: mType = MB_ICONERROR; break;
	case IDOK:mType = MB_ICONASTERISK; break;
	}
	MessageBox(hWnd, (LPCWSTR)text, L"Message               ", MB_OK | mType);
}

enum WindowsOS {
	NotFind,
	Win2000,
	WinXP,
	WinVista,
	Win7,
	Win8,
	Win10
};

WindowsOS GetOsVersion()
{
	using namespace std;
	double ret = 0.0;
	NTSTATUS(WINAPI * RtlGetVersion)(LPOSVERSIONINFOEXW);
	OSVERSIONINFOEXW osInfo;

	ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEXW));
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

	*(FARPROC*)&RtlGetVersion = GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion");

	if (NULL != RtlGetVersion)
	{
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);
		RtlGetVersion(&osInfo);
		ret = (double)osInfo.dwMajorVersion;
	}
	if (osInfo.dwMajorVersion == 10 && osInfo.dwMinorVersion == 0)
	{
		wrlog("this is windows 10\n");
		return Win10;
	}
	else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 3)
	{
		wrlog("this is windows 8.1\n");
		return Win8;
	}
	else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 2)
	{
		wrlog("this is windows 8\n");
		return Win8;
	}
	else if (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 1)
	{
		wrlog("this is windows 7 or Windows Server 2008 R2\n");
		return Win7;
	}

	return NotFind;
}

//========================================================================
// Popup a Windows Error Message, Allegro Style
//========================================================================
void allegro_message(const char* title, const char* message)
{
	MessageBoxA(NULL, message, title, MB_ICONEXCLAMATION | MB_OK);
}

//========================================================================
// Return a std string with last error message
//========================================================================
std::string GetLastErrorStdStr()
{
	DWORD error = GetLastError();
	if (error)
	{
		LPVOID lpMsgBuf;
		DWORD bufLen = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		if (bufLen)
		{
			LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
			std::string result(lpMsgStr, lpMsgStr + bufLen);

			LocalFree(lpMsgBuf);

			return result;
		}
	}
	return std::string();
}

//========================================================================
// Return the Window Handle
//========================================================================
HWND win_get_window()
{
	return hWnd;
}

HRESULT DisableNCRendering(HWND hWnd)
{
	HRESULT hr = S_OK;

	DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;

	// Disable non-client area rendering on the window.
	hr = ::DwmSetWindowAttribute(hWnd,
		DWMWA_NCRENDERING_POLICY,
		&ncrp,
		sizeof(ncrp));

	if (SUCCEEDED(hr))
	{
		// ...
	}

	return hr;
}

void CenterWindow(void)
{
	//   long lWS;
   //    long lWSEX;

	HWND wnd = win_get_window();
	HWND desktop = GetDesktopWindow();
	RECT wndRect, desktopRect;
	int  w, h, dw, dh;

	GetWindowRect(wnd, &wndRect);
	GetWindowRect(desktop, &desktopRect);
	w = wndRect.right - wndRect.left;
	h = wndRect.bottom - wndRect.top;
	dw = desktopRect.right - desktopRect.left;
	dh = desktopRect.bottom - desktopRect.top;

	MoveWindow(wnd, (dw - w) / 2, (dh - h) / 2, w, h, TRUE);
}

void SetWindowForeground(void)
{
	HWND hCurrWnd;
	int iMyTID;
	int iCurrTID;

	hCurrWnd = win_get_window();
	iMyTID = GetCurrentThreadId();
	iCurrTID = GetWindowThreadProcessId(hCurrWnd, 0);

	AttachThreadInput(iMyTID, iCurrTID, TRUE);
	SetForegroundWindow(hCurrWnd);
	AttachThreadInput(iMyTID, iCurrTID, FALSE);
}

void AdjustWindowRectForBorders(const int borders, const int x, const int y,
	const int width, const int height, RECT& r)
{
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	if (borders)
	{
		style |= WS_OVERLAPPEDWINDOW;
	}
	else
	{
		style |= WS_POPUP;
	}
	r.left = x;
	r.top = y;
	r.right = r.left + width;
	r.bottom = r.top + height;
	BOOL result = AdjustWindowRect(&r, style, FALSE);
	if (!result)
	{
		wrlog("AdjustWindowRect failed, error: ");
	}
}

// WinMain
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	WNDCLASS wc;

	//Added in
	DWORD dwStyle, dwExStyle;
	int x, y, width, height;
	int adj;
	RECT WorkArea{};
	RECT rect;
	MSG msg;
	BOOL quit = FALSE;
	//Buffer for command line parsing
	std::vector<std::string> my_argv_buf;
	std::vector<char*> my_argv;

	// register window class
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"Emulator";

	LogOpen("testlog.txt");
	wrlog("Opening Log");

	//This has to be set BEFORE Window creation on Windows 10. This enables or disables DPI Scaling support on Windows 10 or 11
	// Windows 11 still reports as Windows 10. :(
	if (GetOsVersion() == Win10)
	{
		//Make the OS DPI Aware for those people with 4K monitors using scaling.
		BOOL REZ = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
		//DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 // DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 //DPI_AWARENESS_CONTEXT_UNAWARE
		//SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
	}

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	// These are setup for Window mode, need tro add different ones for a full screen Window.
	dwStyle = WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	dwExStyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
	//Adjust the window to account for the frame
	RECT wr = { 0, 0, SCREEN_W, SCREEN_H };
	// Set the size to account for the title bar and borders (really important)
	AdjustWindowRect(&wr, dwStyle, FALSE);

	// Get window size and position as maximized
	SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0);
	x = WorkArea.left;
	y = WorkArea.top;
	//WorkArea.bottom += 8;
	width = (WorkArea.right - WorkArea.left);
	height = (WorkArea.bottom - WorkArea.top);

	//Get the type of Window we want to make.
	SetIniFile("./emulator.ini");
	int sz = get_config_int("main", "winsize", 0);
	if (sz > 1) sz = 1;
	double winsize[] = { 1.25,.75 };

	//adjust for 4:3 window sizing based on the screen resolution - This is assuming a square pixel ratio , needs fixed
	// 1.25 is a normal 4:3 screen wide, .75 is a rotated screen, need to check config, or have this passed as a parameter.
	if ((width - x) > (height * winsize[sz])) //1.25 /.75
	{
		adj = (int)((double)height * (double)winsize[sz]); //1.25 .75
		x += (int)((width - adj) / 2.0);
		width -= (width - adj);
	}
	SetRect(&rect, 0, 0, width, height);
	AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);

	// Create The Window
	if (!(hWnd = CreateWindowEx(dwExStyle,							// Extended Style For The Window
		L"EMULATOR",						// Class Name
		L"EMU Example",					    // Window Title
		dwStyle,					        // Required Window Style
		x, y,   				// Window Position
		width,	// Calculate Window Width
		height,	// Calculate Window Height
		NULL,								// No Parent Window
		NULL,								// No Menu
		hInstance,							// Instance
		NULL)))								// Dont Pass Anything To WM_CREATE
	{
		// Reset The Display
		MessageBox(NULL, L"Window Creation Error.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	// Make sure the window shows up.
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);

	// This has to be done AFTER the window instantiation.
	// Disable Windows 10 window decorations cause I hates them.
	if (GetOsVersion() == Win10)
	{
		// Try to disable Windows 10/11 Decorations. We don't want em.
		// If not, add the 8 pixels back to the bottom and resize?
		DisableNCRendering(hWnd);
	}

	///////////////// Initialize everything here //////////////////////////////
	// Get the actual screen area for OpenGL.
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int gl_width = clientRect.right - clientRect.left;
	int gl_height = clientRect.bottom - clientRect.top;
	wrlog("Actual Screen Width %d, Actual Screen Height %d", gl_width, gl_height);

	SCREEN_W = gl_width;
	SCREEN_H = gl_height;

	//Init OS Timers
	timeBeginPeriod(1);
	TimerInit();

	//Setup cmd line parsing (Not currently being used, but here so I don't lose it. 
	int w_argc = 0;
	LPWSTR* w_argv = CommandLineToArgvW(GetCommandLineW(), &w_argc);
	if (w_argv)
	{
		my_argv_buf.reserve(w_argc);

		for (int i = 0; i < w_argc; ++i)
		{
			int w_len = lstrlenW(w_argv[i]);
			int len = WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, NULL, 0, NULL, NULL);
			std::string s;
			s.resize(len);
			WideCharToMultiByte(CP_ACP, 0, w_argv[i], w_len, &s[0], len, NULL, NULL);
			my_argv_buf.push_back(s);
			wrlog("string here is %s", s.c_str());
		}
		//Uppercase everything
	//	std::transform(my_argv_buf.begin(), my_argv_buf.end(), my_argv_buf.begin(), ::toupper);
		//Make it an array of Char pointers, for main
		//std::vector<char*> my_argv;
		my_argv.reserve(my_argv_buf.size());

		for (size_t i = 0; i < my_argv_buf.size(); ++i)
			my_argv.push_back(const_cast<char*>(my_argv_buf[i].c_str()));
		//main(my_argv.size(), &my_argv[0]);
		LocalFree(w_argv);
	}

	// enable OpenGL for the window
	OpenGL2Enable();
	// Enable GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		wrlog("Error: %s\n", glewGetErrorString(err));
	}
	wrlog("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	// Set the Swap Interval to 60Hz, or whatever the monitor is set to.
	if (WGLEW_EXT_swap_control)
	{
		osWaitVsync(true);
	}
	// Initalize Rawinput
	if ((RawInput_Initialize(hWnd) != 0))
	{
		// Something is really broken, abort.
		MessageBox(NULL, L"Fail: Unable to init RawInput.", L"ERROR", MB_OK | MB_ICONEXCLAMATION);
		//return FALSE;
	}
	//Setup OpenGL
	ViewOrtho(SCREEN_W, SCREEN_H);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Init Emulator Here.
	emulator_init(__argc, __argv);

	//
	/////////////////// END INITIALIZATION ////////////////////////////////////

	// program main loop
	while (!quit)
	{
		// check for messages
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// handle or dispatch messages
			if (msg.message == WM_QUIT)
			{
				quit = TRUE;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			// Run Emulator Here.
			emulator_run();

			glLineWidth(5.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDisable(GL_TEXTURE_2D);

			glColor4ub(255, 10, 10, 255);
			glLine(0, 0, (float)SCREEN_W, (float)SCREEN_H); //one
			glLine(0, (float)SCREEN_H, (float)SCREEN_W, 0); // tweo
			glLine(0, 0, 0, (float)SCREEN_H); // Left
			glLine(0, (float)SCREEN_H, (float)SCREEN_W, (float)SCREEN_H); // Top
			glLine(0, 0, (float)SCREEN_W, 0); // bottom
			glLine((float)SCREEN_W, 0, (float)SCREEN_W, (float)SCREEN_H); // bottom

			GLSwapBuffers();
		}
	}

	//Shutdown Emulator Here
	emulator_end();
	// shutdown OpenGL
	OpenGLShutDown();
	//Stop the Audio Subsystem and release all loaded samples and streams
	//mixer_end();
	//End our font

	//Shutdown logging
	wrlog("Closing Log");
	LogClose();

	// destroy the window explicitly
	DestroyWindow(hWnd);

	return 0;
}

// Window Procedure

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DPICHANGED:
		return 0;

		/*
		 UINT dpiX = LOWORD(wParam);
			UINT dpiY = HIWORD(wParam);

			// Get the suggested new window rectangle
			RECT* newRect = reinterpret_cast<RECT*>(lParam);

			// Update window size and position based on the new DPI
			SetWindowPos(hwnd,
						 NULL,
						 newRect->left,
						 newRect->top,
						 newRect->right - newRect->left,
						 newRect->bottom - newRect->top,
						 SWP_NOZORDER | SWP_NOACTIVATE);

			// Update any other UI elements or resources that depend on DPI
			// ...
		*/

	case WM_CREATE:
		return 0;

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	case WM_INPUT: { return RawInput_ProcessInput(hWnd, wParam, lParam); return 0; }

	case WM_DESTROY:
		return 0;

	case WM_SYSCOMMAND:
	{
		switch (wParam & 0xfff0)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
		{
			return 0;
		}

		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT)
			{
				SetCursor(NULL);

				return TRUE;
			}
			return 0;

		case WM_CLOSE:
		{
			if (MessageBox(hWnd, (L"Do You Want To Exit?"), (L"Exit?"), MB_YESNO) == IDYES)
			{
				DestroyWindow(hWnd);
				PostQuitMessage(0);
			}
			return 0;
		}
		/*
		case SC_CLOSE:
		{
			//I can add a close hook here to trap close button
			quit = 1;
			PostQuitMessage(0);
			break;
		}
		*/
		// User trying to access application menu using ALT?
		case SC_KEYMENU:
			return 0;
		}
		DefWindowProc(hWnd, message, wParam, lParam);
	}

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			PostQuitMessage(0);
			return 0;
		}
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}