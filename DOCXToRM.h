#pragma once
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <vector>
#include "resource.h"
#include "zip.h"
#pragma comment (lib,"Gdiplus.lib")

enum LogLevel {
	LOG_DEBUG_VERBOSE = 0,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};


extern char * LogBuffer;
constexpr auto LB_SIZE = 10240;

void DoLog(const char* Class, const char* Msg, LogLevel Level = LOG_DEBUG_VERBOSE);

//LRESULT CALLBACK ODStaticWndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam);

