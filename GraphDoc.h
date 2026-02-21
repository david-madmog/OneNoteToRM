#pragma once
#include "framework.h"
#include "secrets.h"

#include <nlohmann/json.hpp>

using njson = nlohmann::json;

template<class PageType> class GraphDoc
{
private:
	nlohmann::json * SendRequestAndAwaitResponse(const wchar_t* URLPath);
	void GetLogonToken();

	std::wstring* Token = NULL;
	char * LoginCode = NULL;

protected:
	std::vector<PageType* > Pages;

public:
	void LoginToMicrosoft(HWND hWnd);
	void SetLoginCode(wchar_t * LoginCodeW);
	int LoadDoc(const char* FileName);
	void Resize(HWND hWnd);


};

