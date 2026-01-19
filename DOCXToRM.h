#pragma once
#include "resource.h"
#include "zip.h"
#include "vector"


enum LogLevel {
	LOG_DEBUG_VERBOSE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

void DoLog(const char* Msg, LogLevel Level = LOG_DEBUG_VERBOSE);

//LRESULT CALLBACK ODStaticWndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam);

