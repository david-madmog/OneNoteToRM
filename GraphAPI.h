#pragma once

#include "framework.h"
#include "secrets.h"
#include "OneNoteToRM.h"

#pragma warning ( push )
#pragma warning( disable : 4005 26819)
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )

using njson = nlohmann::json;

typedef struct one_Section {
	std::wstring Notebook;
	std::wstring Section;
	std::wstring ID;
} ONE_Section;


class GraphAPI
{
private:
	char* Refresh_Token = NULL;
	std::wstring* Token = NULL;
	char* LoginCode = NULL;
	int GetLogonToken();

public:
	GraphAPI();
	~GraphAPI();

	std::wstring* SendRequestAndAwaitResponse(const wchar_t* URLPath);
	std::wstring* PostUpdateAndAwaitResponse(const wchar_t* URLPath, const wchar_t* Body, const wchar_t* Boundary);
	void DeletePage(const wchar_t * URLPath);

	void ListSections(std::vector<ONE_Section>& Sections);


	bool EnsureConnected(void);
	void ResizeLogonWindow(HWND hWnd);
	void LoginToMicrosoft(HWND hWnd);
	void SetLoginCode(wchar_t* LoginCodeW);
};

