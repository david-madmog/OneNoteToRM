#include "framework.h"
#include "GraphAPI.h"
#include "resource.h"

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

#include "OneNoteToRM.h"

#pragma warning ( push )
#pragma warning( disable : 4005 26819)
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )


using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

using njson = nlohmann::json;

static wil::com_ptr<ICoreWebView2Controller> webviewController;
static wil::com_ptr<ICoreWebView2> webview;
static uri_builder URI;


GraphAPI::GraphAPI()
{
    // So, when we're instantiated, get the refresh token and clear the access token
    Refresh_Token = new char[LB_SIZE];
    GetPrivateProfileStringA("OneNote", "RefreshToken", "", Refresh_Token, LB_SIZE, gszIniFileName);
    Token = NULL;
}

GraphAPI::~GraphAPI()
{
    delete Refresh_Token;
    if (Token)
        delete Token;
}

std::wstring* GraphAPI::PostUpdateAndAwaitResponse(const wchar_t* URLPath, const wchar_t* Body, const wchar_t* Boundary) {
    if (!Token) {
        std::wostringstream LB;
        LB << L"Failed to get Auth Token";
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return nullptr;
    }

    std::wostringstream LB;
    LB << L"Sending graph data: " << URLPath;
    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

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

    if (Boundary)
    {
        utility::string_t ContentType(web::http::details::mime_types::multipart_form_data);
        ContentType.append(L"; boundary=");
        ContentType.append(Boundary);
        //utility::string_t ContentType(L"application/xhtml+xml");
        request.headers().set_content_type(ContentType);
    }
    else {
        utility::string_t ContentType(web::http::details::mime_types::application_json);
        request.headers().set_content_type(ContentType);
    }

    request.set_request_uri(URI.to_uri());

    pplx::task<http_response> requestTask = client.request(request);

    http_response response;
    try {
        response = requestTask.get(); // If task is not complete, will wait
    }
    catch (std::exception& ex)
    {
        std::wostringstream LB;
        LB << L"Cannot receive data: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return nullptr;
    }
    if (response.status_code() == status_codes::OK || response.status_code() == status_codes::Created)
    {
        pplx::task<utility::string_t> RespDataTask = response.extract_string(true);
        utility::string_t* RespData = new utility::string_t(RespDataTask.get());
        std::wostringstream LB;
        LB << L"GOT data: " << RespData->substr(0, LB_SIZE - 50);
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
        return RespData;   //!!!CHECK WE DELETE THIS
    }
    else {
        std::wostringstream LB;
        LB << L"Cannot receive data: " << response.reason_phrase();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
    }
    return nullptr;
}


std::wstring* GraphAPI::SendRequestAndAwaitResponse(const wchar_t* URLPath) {
    if (!Token) {
        std::wostringstream LB;
        LB << L"Failed to get Auth Token";
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return nullptr;
    }

    std::wostringstream LB;
    LB << L"Sending graph data: " << URLPath;
    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

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
        std::wostringstream LB;
        LB << L"Cannot receive data: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return nullptr;
    }

    if (response.status_code() == status_codes::OK)
    {
        pplx::task<utility::string_t> RespDataTask = response.extract_string(true); /// need to handle multipart response - sets ignore content type to true
        utility::string_t* RespData = new utility::string_t(RespDataTask.get());
        std::wostringstream LB;
        LB << L"GOT data: " << RespData->substr(0, LB_SIZE - 50);
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
        return RespData; //!!!CHECK WE DELETE THIS
    }
    else {
        std::wostringstream LB;
        LB << L"Cannot receive data: " << response.reason_phrase();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
    }
    return nullptr;
}

void GraphAPI::LoginToMicrosoft(HWND hWnd)
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
                                    wchar_t* QueryString = (wchar_t*)malloc((redirectURI.query().size() + 1) * sizeof(wchar_t));
                                    if (QueryString)
                                    {
                                        wcscpy_s(QueryString, redirectURI.query().size() + 1, redirectURI.query().c_str());
                                        PostMessage(hLoginPopup, WM_DONELOGINTOMS, NULL, (LPARAM)QueryString);
                                    }

                                    std::wostringstream LB;
                                    LB << L"Got there! : " << redirectURI.query();
                                    DoLog("WEBVIEW", LB.str(), LOG_ERROR);
                                }
                                else {
                                    std::wostringstream LB;
                                    LB << L"Nav starting: " << redirectURI.host() << redirectURI.path();
                                    DoLog("WEBVIEW", LB.str(), LOG_ERROR);
                                }



                                return S_OK;
                            }).Get(), &token);
                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());


}

int GraphAPI::GetLogonToken()
{
    // So, do we have a refresh token?
    // If so, attempt to refresh, otherwise, do we have an Auth code?
    bool bRefresh = false;
    if (strlen(Refresh_Token) > 1)
        bRefresh = true;
    else if (LoginCode.empty()) {
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
        std::wostringstream LB;
        LB << L"Cannot receive data: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
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
                std::wostringstream LB;
                LB << L"Got Logon token:=" << Token;
                DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);
            }
            else
            {
                std::wostringstream LB;
                LB << "Logon response doesn't contain token: " << RespData.serialize();
                DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
                return -1;
            }

            if (RespData.has_field(L"refresh_token"))
            {
                std::wstring RT{ std::wstring(RespData[L"refresh_token"].as_string()) };
                Refresh_Token = new char[LB_SIZE];
//                RefreshToken = ws2s(RT);
                size_t Num;
                wcstombs_s(&Num, Refresh_Token, LB_SIZE, RT.c_str(), LB_SIZE);
                std::wostringstream LB;
                LB << "Got Refresh token : " << Refresh_Token;
                DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

                WritePrivateProfileStringA("OneNote", "RefreshToken", Refresh_Token, gszIniFileName);
            }

        }
        else {
            pplx::task<utility::string_t> RespDataTask = response.extract_string();
            utility::string_t RespData = RespDataTask.get();

            std::wostringstream LB;
            LB << "Logon response unexpected type: " << RespData;
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
            return -1;
        }
    }
    else {
        std::wostringstream LB;
        LB << "Cannot receive data: " << response.reason_phrase();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return -1;

    }
    return 0;
}

void GraphAPI::DeletePage(const wchar_t* URLPath)
{
    if (!Token) {
        std::wostringstream LB;
        LB << L"Failed to get Auth Token";
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return ;
    }

    std::wostringstream LB;
    LB << L"Requesting page delete: " << URLPath;
    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

    // Create http_client to send the request.
    http_client client(GraphRoot);

    // Build request URI 
    uri_builder URI;
    URI.set_scheme(L"https");
    URI.set_host(GraphHost);
    URI.set_path(L"v1.0");
    URI.append_path(URLPath);

    utility::string_t s = URI.to_string();   // for debugging

    http_request request(methods::DEL);
    request.headers().add(L"Authorization", *Token);
    request.set_request_uri(URI.to_uri());

    pplx::task<http_response> requestTask = client.request(request);

    http_response response;
    try {
        response = requestTask.get(); // If task is not complete, will wait
    }
    catch (std::exception& ex)
    {
        std::wostringstream LB;
        LB << L"Cannot receive data: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return;
    }

    if (response.status_code() == status_codes::NoContent)
    {
        std::wostringstream LB;
        LB << L"GOT Successful delete response";
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);
    }
    else {
        std::wostringstream LB;
        LB << L"Cannot Delete Page: " << response.reason_phrase();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
    }
    return;

}


void GraphAPI::SetLoginCode(wchar_t* LoginCodeW) {
    //size_t i;
    //size_t Size = std::wcslen(LoginCodeW) + 1;
    //LoginCode = (char*)malloc(Size);
    //wcstombs_s(&i, LoginCode, Size, LoginCodeW, Size - 1);
    LoginCode = ws2s(LoginCodeW);
}

bool GraphAPI::EnsureConnected(void) 
{
    if (!Token) {
        if (GetLogonToken() == -1)
        {
            // We can't logon: no refresh token and no access token
            return false;
        }
    }
    return true;
}

void GraphAPI::ResizeLogonWindow(HWND hWnd) {
    if (webviewController != nullptr) {
        RECT bounds;
        GetClientRect(hWnd, &bounds);
        bounds.right = bounds.left + 500;
        webviewController->put_Bounds(bounds);
    }
}

void GraphAPI::ListSections(std::vector<ONE_Section>& Sections) {
    // Get full list of sections and get the ID of the one which matches our input
    std::wstring* RespData = SendRequestAndAwaitResponse(L"me/onenote/sections?$select=id,displayName&$expand=parentNotebook($select=id,displayName)");

    if (!RespData)
        return;

    njson respJson;
    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return;
    }

    if (respJson.contains("value")) {
        for (njson& Section : respJson["value"])
        {
            if (Section.contains("displayName") &&
                Section.contains("parentNotebook") &&
                Section["parentNotebook"].contains("displayName")
                )
            {
                ONE_Section SectionDets;
                std::string XXX = Section.dump(4);
                std::wstringstream tmp;
                tmp << Section["displayName"].get< std::string>().c_str();
                SectionDets.Section = tmp.str() ;
                tmp.str(L"");
                tmp << Section["parentNotebook"]["displayName"].get< std::string>().c_str();
                SectionDets.Notebook = tmp.str();
                tmp.str(L"");
                tmp << Section["id"].get< std::string>().c_str();
                SectionDets.ID = tmp.str();
            
                Sections.push_back(SectionDets);
            }
        }
    }
}


