#pragma once
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

class OAuth
{
public:
	static void ResizeLogonWindow(HWND hWnd);
	static void LoginToMicrosoft(HWND hWnd);
};

