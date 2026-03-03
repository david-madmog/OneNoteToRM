#include "GraphDoc.h"
#include "OneNoteToRM.h"

#pragma warning ( push )
#pragma warning( disable : 26439 26495)
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/filestream.h>
#include <cpprest/asyncrt_utils.h>

#include <Shlwapi.h>
#include <wrl.h>
#include <wil/com.h>
#include <webview2.h>
#pragma warning ( pop )

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

//using json = nlohmann::json;

static wil::com_ptr<ICoreWebView2Controller> webviewController;
static wil::com_ptr<ICoreWebView2> webview;
static uri_builder URI;

// We need to include possible types so compiler knows what instantiations we need
#include "WindowONEPage.h"
template GraphDoc<WindowONEPage>::GraphDoc();
template void GraphDoc<WindowONEPage>::LoginToMicrosoft(HWND hWnd);
template void GraphDoc<WindowONEPage>::SetLoginCode(wchar_t* LoginCodeW);
template int GraphDoc<WindowONEPage>::LoadDoc(const std::string& NotebookName, const std::string& SectionName);
template int GraphDoc<WindowONEPage>::SaveDoc(const std::string& NotebookName, const std::string& SectionName);
template void GraphDoc<WindowONEPage>::Resize(HWND hWnd);
template void GraphDoc<WindowONEPage>::DrawPage(void* DrawDetails, int Page);


template<class PageType> GraphDoc<PageType>::GraphDoc() {
    // So, when we're instantiated, get the refresh token and clear the access token
    Refresh_Token = (char*)malloc(LB_SIZE);
    GetPrivateProfileStringA("OneNote", "RefreshToken", "", Refresh_Token, LB_SIZE, gszIniFileName);
    Token = NULL;
}

template<class PageType> std::wstring* GraphDoc<PageType>::PostUpdateAndAwaitResponse(const wchar_t* URLPath, const wchar_t* Body, const wchar_t* Boundary) {
    if (!Token) {
        sprintf_s(LogBuffer, LB_SIZE, "Failed to get Auth Token");
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        return nullptr;
    }

    sprintf_s(LogBuffer, LB_SIZE, "Sending graph data: %ws", URLPath);
    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);

    // Create http_client to send the request.
    http_client client(GraphRoot);

    // Build request URI 
    uri_builder URI;
    URI.set_scheme(L"https");
    URI.set_host(GraphHost);
    URI.set_path(L"v1.0");
    URI.append_path(URLPath);

    utility::string_t s = URI.to_string();   // for debugging

    http_request request(methods::POST);
    request.headers().add(L"Authorization", *Token);
    request.set_body(Body);
    request.headers().set_content_length(wcslen(Body));
    
    utility::string_t ContentType(web::http::details::mime_types::multipart_form_data);
    ContentType.append(L"; boundary=");
    ContentType.append(Boundary);
    //utility::string_t ContentType(L"application/xhtml+xml");


    request.headers().set_content_type(ContentType);
    request.set_request_uri(URI.to_uri());

    pplx::task<http_response> requestTask = client.request(request);

    http_response response;
    try {
        response = requestTask.get(); // If task is not complete, will wait
    }
    catch (std::exception& ex)
    {
        sprintf_s(LogBuffer, LB_SIZE, "Cannot receive data: %s", ex.what());
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        return nullptr;
    }
    if (response.status_code() == status_codes::OK || response.status_code() == status_codes::Created)
    {
        pplx::task<utility::string_t> RespDataTask = response.extract_string(true);
        //        utility::string_t tmp = ;
        utility::string_t* RespData = new utility::string_t(RespDataTask.get());
        sprintf_s(LogBuffer, LB_SIZE, "GOT data: %ws", RespData->substr(0, LB_SIZE - 50).c_str()); // response can be quite long!
        DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
        return RespData;
    }
    else {
        reason_phrase Reason = response.reason_phrase();
        sprintf_s(LogBuffer, LB_SIZE, "Cannot receive data: %ws", Reason.c_str());
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
    }
    return nullptr;
}


template<class PageType> std::wstring* GraphDoc<PageType>::SendRequestAndAwaitResponse(const wchar_t* URLPath) {
    if (!Token) {
        sprintf_s(LogBuffer, LB_SIZE, "Failed to get Auth Token");
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        return nullptr;
    }

    sprintf_s(LogBuffer, LB_SIZE, "Requesting graph data: %ws", URLPath);
    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);

    // Create http_client to send the request.
    http_client client(GraphRoot);

    // Build request URI 
    uri_builder URI;
    URI.set_scheme(L"https");
    URI.set_host(GraphHost);
    URI.set_path(L"v1.0");
    URI.append_path(URLPath);

    utility::string_t s = URI.to_string();   // for debugging

    http_request request(methods::GET);
    request.headers().add(L"Authorization", *Token);
    request.set_request_uri(URI.to_uri());

    pplx::task<http_response> requestTask = client.request(request);

    http_response response;
    try {
        response = requestTask.get(); // If task is not complete, will wait
    }
    catch (std::exception& ex)
    {
        sprintf_s(LogBuffer, LB_SIZE, "Cannot receive data: %s", ex.what());
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        return nullptr;
    }

    if (response.status_code() == status_codes::OK)
    {
        pplx::task<utility::string_t> RespDataTask = response.extract_string(true); /// need to handle multipart response - sets ignore content type to true
//        utility::string_t tmp = ;
        utility::string_t * RespData = new utility::string_t(RespDataTask.get());
        sprintf_s(LogBuffer, LB_SIZE, "GOT data: %ws", RespData->substr(0, LB_SIZE - 50).c_str()); // response can be quite long!
        DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
        return RespData;
    }
    else {
        reason_phrase Reason = response.reason_phrase();
        sprintf_s(LogBuffer, LB_SIZE, "Cannot receive data: %ws", Reason.c_str());
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
    }
    return nullptr;
}

template<class PageType> void GraphDoc<PageType>::LoginToMicrosoft(HWND hWnd)
{
    char* code = nullptr;
    // SO we build the URL, and then use the WebView to get permission/login

    URI.set_scheme(L"https");
    URI.set_host(EndpointHost);
    URI.set_path(TenantID);
    URI.append_path(OAuthEndpoint);

    URI.set_query(L"");
    URI.append_query(L"client_id", EntraAppID, true);
    URI.append_query(L"response_type", L"code", true);
    URI.append_query(L"redirect_uri", RedirectURI, false);
    URI.append_query(L"response_mode", L"query", true);
//    URI.append_query(L"scope", L"https://graph.microsoft.com/.default offline_access notes.Create notes.ReadWrite", true);
    URI.append_query(L"scope", L"offline_access Notes.Create Notes.Read Notes.ReadWrite Notes.Read.All Notes.ReadWrite.All", true);

    utility::string_t s = URI.to_string();   // for debugging
    // Step 3 - Create a single WebView within the parent window
        // Locate the browser and set up the environment for WebView
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

                // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                env->CreateCoreWebView2Controller(hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hWnd](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (controller != nullptr) {
                            webviewController = controller;
                            webviewController->get_CoreWebView2(&webview);
                        }

                        // Add a few settings for the webview
                        wil::com_ptr<ICoreWebView2Settings> settings;
                        webview->get_Settings(&settings);
                        settings->put_IsScriptEnabled(TRUE);
                        settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                        settings->put_IsWebMessageEnabled(TRUE);


                        // Schedule an async task to navigate to Our Logon site
                        webview->Navigate(URI.to_uri().to_string().c_str());

                        EventRegistrationToken token;
                        // SO, this is how it works... 
                        // We display the navi site, and it will redirect us to the "Redirect URI" once we're done with the
                        // login code in the query string. So, we look at where we're navigating to, and if it's there, we
                        // can extract the code and close ourselves
                        webview->add_NavigationStarting(Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                        	[](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                        		wil::unique_cotaskmem_string uri;
                        		args->get_Uri(&uri);
                        		std::wstring source(uri.get());

                                web::uri redirectURI(source);
                                if (redirectURI.host() == RedirectURIHost && redirectURI.path() == RedirectURIPath) {
                                    wchar_t* QueryString = (wchar_t*) malloc((redirectURI.query().size() + 1) * sizeof(wchar_t));
                                    if (QueryString)
                                    {
                                        wcscpy_s(QueryString, redirectURI.query().size() + 1, redirectURI.query().c_str());
                                        PostMessage(hLoginPopup, WM_DONELOGINTOMS, NULL, (LPARAM)QueryString);
                                    }

                                    sprintf_s(LogBuffer, LB_SIZE, "Got there! : %ws", redirectURI.query().c_str());
                                    DoLog("WEBVIEW", LogBuffer, LOG_INFO);
                                } else {
                                    sprintf_s(LogBuffer, LB_SIZE, "Nav starting: %ws%ws", redirectURI.host().c_str(), redirectURI.path().c_str());
                                    DoLog("WEBVIEW", LogBuffer, LOG_INFO);
                                }

                               
                                
                                return S_OK;
                        	}).Get(), &token);
                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());


}

template<class PageType> int GraphDoc<PageType>::GetLogonToken()
{
    // So, do we have a refresh token?
    // If so, attempt to refresh, otherwise, do we have an Auth code?
    bool bRefresh = false;
    if (strlen(Refresh_Token) > 1)
        bRefresh = true;
    else if (!LoginCode) {
        // We have neither - give up
        return -1;
    }

    http_client client(EndpointRoot);

    URI.set_path(TenantID);
    URI.append_path(OAuthTokenEndpoint);

  //     utility::string_t s = URI.to_string();   // for debugging

    utf8string body;
    body.append("client_id="); body.append(EntraAppID);
    //    body.append("&scope=https%3A%2F%2Fgraph.microsoft.com%2F.default");
    body.append("&scope=offline_access%20Notes.Create%20Notes.Read%20Notes.ReadWrite%20Notes.Read.All%20Notes.ReadWrite.All");
    body.append("&redirect_uri="); body.append(RedirectURI);
    if (bRefresh)
    {
        body.append("&refresh_token="); body.append(Refresh_Token);
        body.append("&grant_type=refresh_token");
    }
    else {
        body.append("&"); body.append(LoginCode);
        body.append("&grant_type=authorization_code");
    }

    http_request request(methods::POST);
    request.set_body(body);
    request.headers().set_content_length(body.length());
    request.headers().set_content_type(web::http::details::mime_types::application_x_www_form_urlencoded);
    request.set_request_uri(URI.to_uri());

    pplx::task<http_response> requestTask = client.request(request);
    http_response response;
    try {
        response = requestTask.get(); // If task is not complete, will wait
    }
    catch (std::exception& ex)
    {
        sprintf_s(LogBuffer, LB_SIZE, "Cannot receive data: %s", ex.what());
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        return -1;
    }

    if (response.status_code() == status_codes::OK)
    {
        //        std::wstring ss = response.headers().content_type();
        //        utility::string_t st = web::http::details::mime_types::application_json;

        if (response.headers().content_type().find(web::http::details::mime_types::application_json) != std::string::npos)
        {
            pplx::task<web::json::value> RespDataTask = response.extract_json();
            web::json::value RespData = RespDataTask.get();

            utility::string_t DD = RespData.serialize();

            if (RespData.has_field(L"access_token"))
            {
                Token = new std::wstring(RespData[L"access_token"].as_string());
                Token->insert(0, L"Bearer ");
                sprintf_s(LogBuffer, LB_SIZE, "Got Logon token: %ws", Token->c_str());
                DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);
            }
            else
            {
                sprintf_s(LogBuffer, LB_SIZE, "Logon response doesn't contain token: %ws", RespData.serialize().c_str());
                DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
                return -1;
            }

            if (RespData.has_field(L"refresh_token"))
            {
                std::wstring RT{ std::wstring(RespData[L"refresh_token"].as_string()) };
                Refresh_Token = (char*)malloc(LB_SIZE);
                size_t Num;
                wcstombs_s(&Num, Refresh_Token, LB_SIZE, RT.c_str(), LB_SIZE);
                sprintf_s(LogBuffer, LB_SIZE, "Got Refresh token: %s", Refresh_Token);
                DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);
                WritePrivateProfileStringA("OneNote", "RefreshToken", Refresh_Token, gszIniFileName);
            }

        }
        else {
            pplx::task<utility::string_t> RespDataTask = response.extract_string();
            utility::string_t RespData = RespDataTask.get();

            sprintf_s(LogBuffer, LB_SIZE, "Logon response unexpected type: %ws", RespData.c_str());
            DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
            return -1;
        }
    }
    else {

        reason_phrase Reason = response.reason_phrase();
        sprintf_s(LogBuffer, LB_SIZE, "Cannot receive data: %ws", Reason.c_str());
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        return -1;

    }
    return 0;
}

template<class PageType> void GraphDoc<PageType>::SetLoginCode(wchar_t* LoginCodeW) {
    size_t i;
    size_t Size = std::wcslen(LoginCodeW) + 1;
    LoginCode = (char*)malloc(Size);
    wcstombs_s(&i, LoginCode, Size, LoginCodeW, Size-1 );
}

template<class PageType> int GraphDoc<PageType>::LoadPages(wchar_t * SectionID) {

    // First get the list of pages
    std::wstring PagesList = L"me/onenote/sections/";
    PagesList.append(SectionID);
    PagesList.append(L"/pages?$select=id,title");

    std::wstring* RespData = SendRequestAndAwaitResponse(PagesList.c_str());
    if (RespData == nullptr)
        return 0;

    njson respJson;
    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        sprintf_s(LogBuffer, LB_SIZE, "JSON Parse Error: %s", ex.what());
        DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
        return 0;
    }

    if (respJson.contains("value")) {
        size_t convertedChars = 0;
        wchar_t LocalWBuff[1024];

        // Now we have the list of pages, get the content for each one
		for (njson& PageJson : respJson["value"])
        {
            std::wstring PageQuery{ L"me/onenote/pages/" };
            std::string ID = PageJson["id"].get< std::string>();
            mbstowcs_s(&convertedChars, LocalWBuff, 1024, ID.c_str(), ID.length());
            PageQuery.append(LocalWBuff);
            PageQuery.append(L"/content?includeinkML=true");
            std::wstring* PageData = SendRequestAndAwaitResponse(PageQuery.c_str());
            if (PageData)
            {
                PageType* Page = new PageType;
                Pages.push_back(Page);
                std::string Title{ PageJson["title"].get< std::string>() };
                Page->LoadPage(PageData, Title);
            }
        }
    }

    return (int)Pages.size();
}


template<class PageType> wchar_t * GraphDoc<PageType>::FindDocID(const std::string& NotebookName,const std::string& SectionName)
{
    // Get full list of sections and get the ID of the one which matches our input
    std::wstring* RespData = SendRequestAndAwaitResponse(L"me/onenote/sections?$select=id,displayName&$expand=parentNotebook($select=id,displayName)");
    njson respJson;
    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        sprintf_s(LogBuffer, LB_SIZE, "JSON Parse Error: %s", ex.what());
        DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
        return 0;
    }

    if (respJson.contains("value")) {
        for (njson & Section : respJson["value"]) 
        {
            if (Section.contains("displayName") &&
                Section.contains("parentNotebook") &&
                Section["parentNotebook"].contains("displayName")
                ) 
            {
                std::string XXX = Section.dump(4);
                std::string FoundSectionName = Section["displayName"].get< std::string>() ;
                std::string FoundNotebookName = Section["parentNotebook"]["displayName"].get< std::string>();

                if (SectionName== FoundSectionName && NotebookName==FoundNotebookName) {
                    // Hooray - found what we're looking for
                    std::string SectionID = Section["id"].get< std::string>();
                    size_t convertedChars = 0;
                    wchar_t* LocalWBuff = (wchar_t*)malloc((SectionID.length() + 1) * sizeof(wchar_t));
                    mbstowcs_s(&convertedChars, LocalWBuff, SectionID.length() + 1, SectionID.c_str(), SectionID.length());
                    return LocalWBuff;
                }
            }
        }
    }


    return 0;
}

template<class PageType> int GraphDoc<PageType>::LoadDoc(const std::string& NotebookName, const std::string& SectionName)
{
    if (!Token) {
        if (GetLogonToken() == -1)
        {
            // We can't logon: no refresh token and no access token
            return -1;
        }
    }

    wchar_t* SectionID = FindDocID(NotebookName, SectionName);
    if (SectionID)
    {
        int NumPages = LoadPages(SectionID);
        free(SectionID);
        return NumPages;
    }
    return 0;
}

template<class PageType> int GraphDoc<PageType>::SaveDoc(const std::string& NotebookName, const std::string& SectionName)
{
    wchar_t* SectionID = FindDocID(NotebookName, SectionName);
    if (SectionID)
    {
        nonce_generator NonceGen;
        for ( auto& Page : Pages) {
            utility::string_t Nonce = NonceGen.generate();

            std::wstring* PageData = Page->SavePage(Nonce);

            std::wstring PageURL = L"me/onenote/sections/";
            PageURL.append(SectionID);
            PageURL.append(L"/pages");

            std::wstring* RespData = PostUpdateAndAwaitResponse(PageURL.c_str(), PageData->c_str(), Nonce.c_str());
            if (RespData)
            {
                njson respJson;
                try {
                    respJson = njson::parse(*RespData);
                }
                catch (njson::parse_error ex) {
                    sprintf_s(LogBuffer, LB_SIZE, "JSON Parse Error: %s", ex.what());
                    DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
                    return 0;
                }
            }

            delete PageData;
        }

        //int NumPages = LoadPages(SectionID);
        free(SectionID);
    }
    return 0;
}

template<class PageType> void GraphDoc<PageType>::Resize(HWND hWnd) {
    if (webviewController != nullptr) {
        RECT bounds;
        GetClientRect(hWnd, &bounds);
        bounds.right = bounds.left + 500;
        webviewController->put_Bounds(bounds);
    }
}

template<class PageType> void GraphDoc<PageType>::DrawPage(void* DrawDetails, int Page)
{
    if (Page < Pages.size())
    {
        ONEPage* P = Pages[Page];
        P->DrawPage(DrawDetails);
    }
}


/*
* 
* 
* 
* 
* 
me/onenote/sections?$select=id,displayName&$expand=parentNotebook($select=id,displayName)
List of sections RESPONSE
{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections(id,displayName,parentNotebook(id,displayName))",
    "value": [
        {
            "id": "0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "displayName": "New Section 1",
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/parentNotebook(id,displayName)/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
                "displayName": "TestNotebook 2"
            }
        },
        {
            "id": "0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0",
            "displayName": "New Section 1",
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21sc9911f43ef4e4e3381d49eb4214618d0')/parentNotebook(id,displayName)/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62",
                "displayName": "Test1"
            }
        },
        {
            "id": "0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61",
            "displayName": "New Section 1",
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s4cc1e448591a40c39821dd696b95ad61')/parentNotebook(id,displayName)/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
                "displayName": "test3"
            }
        }
    ]
}

    //https://graph.microsoft.com/v1.0/me/onenote/sections/0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df/pages?$select=id,title


{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/pages(id,title)",
    "value": [
        {
            "id": "0-89b0f965a4f4434ba7d479926c2c1f82!1-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "title": "Second page"
        },
        {
            "id": "0-5b84d480aca444a4bb0c96faf54de213!1-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "title": "Test Notebook 2"
        }
    ]
}




{   "@odata.context":"https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/notebooks", 
    "value" : [
        {   "id":"0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62", 
            "self" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62", 
            "createdDateTime" : "2026-02-10T16:26:33Z", 
            "displayName" : "Test1", 
            "lastModifiedDateTime" : "2026 - 02 - 10T16 : 26 : 33Z", 
            "isDefault" : false, 
            "userRole" : "Owner", 
            "isShared" : false, 
            "sectionsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62/sections", 
            "sectionGroupsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62/sectionGroups", 
            "createdBy" : {"user":{"id":"ADAEA281180757D1", "displayName" : "David Poirier"}}, 
            "lastModifiedBy" : {"user":{"id":"ADAEA281180757D1", "displayName" : "David Poirier"}}, 
            "links" : {
                "oneNoteClientUrl":{"href":"onenote:https://d.docs.live.net/adaea281180757d1/Documents/Development/ReMarkable/DOCXToRM/Test1"}, 
                "oneNoteWebUrl" : {"href":"https://onedrive.live.com/redir.aspx?resid=ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62&id=documents&page=edit&cid=adaea281180757d1"}
            }
        },
        {   "id":"0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
            "self" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
            "createdDateTime" : "2026-02-10T16:37:10Z",
            "displayName" : "test3",
            "lastModifiedDateTime" : "2026-02-10T16:37:10Z",
            "isDefault" : false,
            "userRole" : "Owner",
            "isShared" : false,
            "sectionsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c/sections",
            "sectionGroupsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c/sectionGroups",
            "createdBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "lastModifiedBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "links" : {
                "oneNoteClientUrl":{"href":"onenote:https://d.docs.live.net/adaea281180757d1/Documents/Development/ReMarkable/DOCXToRM/test3"},
                "oneNoteWebUrl" : {"href":"https://onedrive.live.com/redir.aspx?resid=ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c&id=documents&page=edit&cid=adaea281180757d1"}
            } 
        }, 
        {   "id":"0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
            "self" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
            "createdDateTime" : "2026-02-20T15:54:20Z",
            "displayName" : "TestNotebook 2",
            "lastModifiedDateTime" : "2026-02-20T15:54:20Z",
            "isDefault" : false,
            "userRole" : "Owner",
            "isShared" : false,
            "sectionsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3/sections",
            "sectionGroupsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3/sectionGroups",  
            "createdBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "lastModifiedBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "links" : {
                "oneNoteClientUrl":{"href":"onenote:https://d.docs.live.net/adaea281180757d1/Documents/TestNotebook 2"},
                "oneNoteWebUrl" : {"href":"https://onedrive.live.com/redir.aspx?resid=ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3&id=documents&page=edit&cid=adaea281180757d1"}
            } 
        }
    ] 
}


{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections",
    "@microsoft.graph.tips": "Use $select to choose only the properties your app needs, as this can lead to performance improvements. For example: GET me/onenote/sections?$select=isDefault,links",
    "value": [
        {
            "id": "0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "createdDateTime": "2026-02-20T15:55:30Z",
            "displayName": "New Section 1",
            "lastModifiedDateTime": "2026-02-20T15:55:33Z",
            "isDefault": false,
            "pagesUrl": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df/pages",
            "createdBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "lastModifiedBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/parentNotebook/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
                "displayName": "TestNotebook 2",
                "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3"
            },
            "parentSectionGroup@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/parentSectionGroup/$entity",
            "parentSectionGroup": null
        },
        {
            "id": "0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0",
            "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0",
            "createdDateTime": "2026-02-10T16:26:46Z",
            "displayName": "New Section 1",
            "lastModifiedDateTime": "2026-02-10T16:28:09Z",
            "isDefault": false,
            "pagesUrl": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0/pages",
            "createdBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "lastModifiedBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21sc9911f43ef4e4e3381d49eb4214618d0')/parentNotebook/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62",
                "displayName": "Test1",
                "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62"
            },
            "parentSectionGroup@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21sc9911f43ef4e4e3381d49eb4214618d0')/parentSectionGroup/$entity",
            "parentSectionGroup": null
        },
        {
            "id": "0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61",
            "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61",
            "createdDateTime": "2026-02-20T15:55:19Z",
            "displayName": "New Section 1",
            "lastModifiedDateTime": "2026-02-20T15:55:23Z",
            "isDefault": false,
            "pagesUrl": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61/pages",
            "createdBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "lastModifiedBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s4cc1e448591a40c39821dd696b95ad61')/parentNotebook/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
                "displayName": "test3",
                "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c"
            },
            "parentSectionGroup@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s4cc1e448591a40c39821dd696b95ad61')/parentSectionGroup/$entity",
            "parentSectionGroup": null
        }
    ]
}


*/