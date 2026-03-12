#pragma once
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include "framework.h"
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

extern HWND hLoginPopup;
extern char gszIniFileName[];

extern char * LogBuffer;
constexpr auto LB_SIZE = 10240;

void DoLog(const char* Class, const char* Msg, LogLevel Level = LOG_DEBUG_VERBOSE);
void DoLog(const char* Class, const wchar_t * Msg, LogLevel Level = LOG_DEBUG_VERBOSE);

class Drawable {
public:
	virtual void DrawPage(void* DrawDetails, int page) {};
};

struct DrawDetailsParams {
	HDC hDC;
	RECT Rect;
};