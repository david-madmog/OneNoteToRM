#pragma once
#include "framework.h"
#include "secrets.h"

#pragma warning ( push )
#pragma warning( disable : 4005 )
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )

using njson = nlohmann::json;

template<class PageType> class GraphDoc
{
private:
	std::wstring * SendRequestAndAwaitResponse(const wchar_t* URLPath);
	int GetLogonToken();
	int LoadPages(wchar_t* SectionID);

	char * Refresh_Token = NULL;
	std::wstring* Token = NULL;
	char * LoginCode = NULL;

protected:
	std::vector<PageType* > Pages;

public:
	GraphDoc();
	void LoginToMicrosoft(HWND hWnd);
	void SetLoginCode(wchar_t * LoginCodeW);
	int LoadDoc(const std::string& NotebookName, const std::string& SectionName);
	void Resize(HWND hWnd);

	void DrawPage(void* DrawDetails, int page);

};

