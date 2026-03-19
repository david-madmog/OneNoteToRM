#pragma once

#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

/*******************************************************************************

	GraphAPI.h

	Header for functions to call Microsoft graph API - used to manage OneNote 
		documents. It makes heavy use of the C++ REST SDK for making the calls and
		nlohmann/json parser for building the body of requests and managing the responses

	see https://learn.microsoft.com/en-us/graph/integrate-with-onenote for API 
		description.
	see https://microsoft.github.io/cpprestsdk/classweb_1_1http_1_1http__request.html#a2c74132fbe2df7c5b0982c17e4e85e19
		for documentation on C++ REST API
	see https://json.nlohmann.me/api/basic_json/ for JSON docs

	(C) David Poirier 2026

********************************************************************************/


const char EntraAppID[] = "2e6c6dab-a8ad-46f5-9c00-5e0c7e4dca19";
const wchar_t TenantID[] = L"common";
//const char ClientSecretValue[] = "1Gf8Q~_hLUTu5FLWtPWkZyG0X_H3Me.0WiIYZchL";
//const char ClientSecretID[] = "18d5122c-756b-44a8-994a-3db16cc6b7d4";
const char RedirectURI[] = "https%3A%2F%2Flogin.microsoftonline.com%2Fcommon%2Foauth2%2Fnativeclient";
const wchar_t RedirectURIHost[] = L"login.microsoftonline.com";
const wchar_t RedirectURIPath[] = L"/common/oauth2/nativeclient";

const wchar_t EndpointRoot[] = L"https://login.microsoftonline.com/";
const wchar_t EndpointHost[] = L"login.microsoftonline.com";
const wchar_t OAuthEndpoint[] = L"oauth2/v2.0/authorize";
const wchar_t OAuthTokenEndpoint[] = L"oauth2/v2.0/token";

const wchar_t GraphRoot[] = L"https://graph.microsoft.com/";
const wchar_t GraphHost[] = L"graph.microsoft.com";




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
	//	char* LoginCode = NULL;
	std::string LoginCode ;
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

