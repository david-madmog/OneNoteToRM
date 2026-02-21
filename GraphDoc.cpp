#include "GraphDoc.h"
#include "DOCXToRM.h"

#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/filestream.h>

#include <Shlwapi.h>
#include <wrl.h>
#include <wil/com.h>
#include <webview2.h>

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
template void GraphDoc<WindowONEPage>::LoginToMicrosoft(HWND hWnd);
template void GraphDoc<WindowONEPage>::SetLoginCode(wchar_t* LoginCodeW);
template int GraphDoc<WindowONEPage>::LoadDoc(const char* FileName);
template void GraphDoc<WindowONEPage>::Resize(HWND hWnd);


template<class PageType> nlohmann::json * GraphDoc<PageType>::SendRequestAndAwaitResponse(const wchar_t * URLPath){
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
        pplx::task<utility::string_t> RespDataTask = response.extract_string();
        utility::string_t RespData = RespDataTask.get();
        
        njson * json = new njson(njson::parse(RespData));

        sprintf_s(LogBuffer, LB_SIZE, "GOT data: %ws", RespData.c_str());
        DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
        return json;
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

    // OK, first we need to do an authorisation on behalf of the user to get the user consent
    //GET https://login.microsoftonline.com/{tenant}/oauth2/v2.0/authorize?
    //client_id=11111111-1111-1111-1111-111111111111
    //&response_type=code
    //&redirect_uri=http%3A%2F%2Flocalhost%2Fmyapp%2F
    //&response_mode=query
    //&scope=offline_access%20user.read%20mail.read
    //&state=12345  HTTP/1.1

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
                        //webview->Navigate(L"https://blanquilla.uk");


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
                                    wcscpy_s(QueryString, redirectURI.query().size() + 1, redirectURI.query().c_str());
                                    PostMessage(hLoginPopup, WM_DONELOGINTOMS, NULL, (LPARAM)QueryString);

                                    sprintf_s(LogBuffer, LB_SIZE, "Got there! : %ws", redirectURI.query().c_str());
                                    DoLog("WEBVIEW", LogBuffer, LOG_INFO);
                                } else {
                                    sprintf_s(LogBuffer, LB_SIZE, "Nav starting: [%ws]//[%ws]", redirectURI.host().c_str(), redirectURI.path().c_str());
                                    DoLog("WEBVIEW", LogBuffer, LOG_INFO);
                                }

                               
                                
                                return S_OK;
                        	}).Get(), &token);
                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());


}

template<class PageType> void GraphDoc<PageType>::GetLogonToken()
{
    // NOW we request an access token...
        // Create http_client to send the request.
    http_client client(EndpointRoot);

    URI.set_path(TenantID);
    URI.append_path(OAuthTokenEndpoint);

  //     utility::string_t s = URI.to_string();   // for debugging

    utf8string body;
    body.append("client_id="); body.append(EntraAppID);
    //    body.append("&scope=https%3A%2F%2Fgraph.microsoft.com%2F.default");
    body.append("&scope=offline_access%20Notes.Create%20Notes.Read%20Notes.ReadWrite%20Notes.Read.All%20Notes.ReadWrite.All");
    body.append("&");
    body.append(LoginCode);
    body.append("&redirect_uri="); body.append(RedirectURI);
    //    body.append("&client_secret="); body.append(ClientSecretValue); // not sure if it's needed, but we have it
    body.append("&grant_type=authorization_code");

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
        return;
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
            }

        }
        else {
            pplx::task<utility::string_t> RespDataTask = response.extract_string();
            utility::string_t RespData = RespDataTask.get();

            sprintf_s(LogBuffer, LB_SIZE, "Logon response unexpected type: %ws", RespData.c_str());
            DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        }
    }
    else {

        reason_phrase Reason = response.reason_phrase();
        sprintf_s(LogBuffer, LB_SIZE, "Cannot receive data: %ws", Reason.c_str());
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);

    }
    
}

template<class PageType> void GraphDoc<PageType>::SetLoginCode(wchar_t* LoginCodeW) {
    size_t i;
    size_t Size = std::wcslen(LoginCodeW) + 1;
    LoginCode = (char*)malloc(Size);
    wcstombs_s(&i, LoginCode, Size, LoginCodeW, Size-1 );
}


template<class PageType> int GraphDoc<PageType>::LoadDoc(const char* FileName)
{
    if (!Token)
        GetLogonToken();
    
//    SendRequestAndAwaitResponse(L"me");
    njson * J = SendRequestAndAwaitResponse(L"me/onenote/notebooks");
//    SendRequestAndAwaitResponse(L"me/photo/$value");
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
