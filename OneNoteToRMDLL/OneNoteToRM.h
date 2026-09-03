#pragma once
#include <windows.h>
#include <string>
#include <fstream>
#include <istream>
#include <iostream>
#include "OneNoteToRMDLL.h"

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

extern char gszIniFileName[];
extern std::wstring AppLocalDirectory;

constexpr auto TOKEN_INI = "\\OneNoteToRMToken.ini";

std::wstring s2ws(const std::unique_ptr<char> str);
std::wstring s2ws(const std::string& str);
std::string ws2s(const std::unique_ptr<wchar_t> str) ;
std::string ws2s(const std::wstring& wstr);

#define LB_SIZE 10240l

void DoLog(const char* Class, const char* Msg, LogLevel Level = LOG_DEBUG_VERBOSE);
void DoLog(const char* Class, const wchar_t* Msg, LogLevel Level = LOG_DEBUG_VERBOSE);
void DoLog(const char* Class, const std::wstring& Msg, LogLevel Level = LOG_DEBUG_VERBOSE);

// Pure abstract base classes
class BaseDoc {
public:
	virtual void DrawPage(void* DrawDetails, int page)=0;
	virtual int LoadDoc(const std::string& Part1, const std::string& Part2) = 0;
	virtual int SaveDoc(const std::string& Part1, const std::string& Part2) = 0;
	virtual int LoadDoc(const wchar_t* SectionID) = 0;
	virtual int SaveDoc(const wchar_t* SectionID) = 0;
	virtual time_t LastEditTime()=0;
	std::string Name = "";
};

class BaseAPI {
public:
	virtual bool EnsureConnected(void) = 0;
	virtual void SetAuthCode(const wchar_t* DeviceCodeW) = 0;
	virtual std::wstring ListDocsString() = 0;
};
