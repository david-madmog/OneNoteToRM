#pragma once

#include <string>
#include <vector>

#include "OneNoteToRM.h"

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

#ifndef IB_SIZE
#define IB_SIZE 1024
#endif // !IB_SIZE


typedef struct one_Section {
	std::wstring Notebook;
	std::wstring Section;
	std::wstring Hash;
} ONE_Section;


class GraphAPI : public BaseAPI
{
private:
	std::string TokenIniFileName;
	char* Refresh_Token = NULL;
	std::wstring* Token = NULL;
	//	char* LoginCode = NULL;
	std::string LoginCode ;
	int GetLogonToken();

	char EntraAppID[IB_SIZE];
	wchar_t TenantID[IB_SIZE];
	char RedirectURI[IB_SIZE];

	wchar_t EndpointRoot[IB_SIZE];
	wchar_t OAuthTokenEndpoint[IB_SIZE];

	wchar_t GraphRoot[IB_SIZE];
	wchar_t GraphHost[IB_SIZE];

public:
	GraphAPI();
	~GraphAPI();

	std::wstring* SendRequestAndAwaitResponse(const wchar_t* URLPath);
	std::wstring* PostUpdateAndAwaitResponse(const wchar_t* URLPath, const wchar_t* Body, const wchar_t* Boundary);
	void DeletePage(const wchar_t * URLPath);

	void ListSections(std::vector<ONE_Section>& Sections);

	// Override base class
	bool EnsureConnected(void);
	void SetAuthCode(const wchar_t* DeviceCodeW);
	std::wstring ListDocsString();
};

