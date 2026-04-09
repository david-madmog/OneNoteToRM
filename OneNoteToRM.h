#pragma once
#include <windows.h>
#include <string>

/*******************************************************************************

	OneNoteToRM.h

	Header for conversion of OneNote format into ReMarkable file blocks

	(C) David Poirier 2026

********************************************************************************/


enum LogLevel {
	LOG_DEBUG_VERBOSE = 0,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

extern HWND hLoginPopup;
extern char gszIniFileName[];

std::wstring s2ws(const std::unique_ptr<char> str);
std::wstring s2ws(const std::string& str);
std::string ws2s(const std::unique_ptr<wchar_t> str) ;
std::string ws2s(const std::wstring& wstr);

//extern char * LogBuffer;
constexpr auto LB_SIZE = 10240;

void DoLog(const char* Class, const char* Msg, LogLevel Level = LOG_DEBUG_VERBOSE);
void DoLog(const char* Class, const wchar_t* Msg, LogLevel Level = LOG_DEBUG_VERBOSE);
void DoLog(const char* Class, const std::wstring& Msg, LogLevel Level = LOG_DEBUG_VERBOSE);

class Drawable {
public:
	virtual void DrawPage(void* DrawDetails, int page) {};
};

struct DrawDetailsParams {
	HDC hDC;
	RECT Rect;
};