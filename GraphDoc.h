#pragma once
#include "framework.h"
#include "secrets.h"

#pragma warning ( push )
#pragma warning( disable : 4005 26819)
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )

using njson = nlohmann::json;

template<class PageType> class GraphDoc
{
private:
	std::wstring* SendRequestAndAwaitResponse(const wchar_t* URLPath);
	std::wstring* PostUpdateAndAwaitResponse(const wchar_t* URLPath, const wchar_t * Body, const wchar_t* Boundary);
	int GetLogonToken();
	int LoadPages(wchar_t* SectionID);
	wchar_t* FindDocID(const std::string& NotebookName, const std::string& SectionName);

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
	int SaveDoc(const std::string& NotebookName, const std::string& SectionName);
	void Resize(HWND hWnd);

	void DrawPage(void* DrawDetails, int page);

	void AddPage(PageType* Page) { Pages.push_back(Page); }
};

