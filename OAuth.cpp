#include "pch.h"
#include "OAuth.h"
#include "resource.h"

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

using namespace web;                        // Common features like URIs.

static wil::com_ptr<ICoreWebView2Controller> webviewController;
static wil::com_ptr<ICoreWebView2> webview;
static uri_builder URI;



void OAuth::LoginToMicrosoft(HWND hWnd)
{
    char* code = nullptr;
    // SO we build the URL, and then use the WebView to get permission/login
    char * EntraAppID = new char[LB_SIZE];
    wchar_t *TenantID = new wchar_t[LB_SIZE];
    char * RedirectURI = new char[LB_SIZE];
    wchar_t * RedirectURIHost = new wchar_t[LB_SIZE];
    wchar_t * RedirectURIPath = new wchar_t[LB_SIZE];

    wchar_t * EndpointHost = new wchar_t[LB_SIZE];
    wchar_t * OAuthEndpoint = new wchar_t[LB_SIZE];

    char* gszIniFileName = GetIniFile();
    std::wstring wIniFileName = s2ws(gszIniFileName);

    GetPrivateProfileStringA("OneNote", "EntraAppID", "", EntraAppID, LB_SIZE, gszIniFileName);
    GetPrivateProfileStringW(L"OneNote", L"TenantID", L"", TenantID, LB_SIZE, wIniFileName.c_str());
    GetPrivateProfileStringA("OneNote", "RedirectURI", "", RedirectURI, LB_SIZE, gszIniFileName);
    GetPrivateProfileStringW(L"OneNote", L"RedirectURIHost", L"", RedirectURIHost, LB_SIZE, wIniFileName.c_str());
    GetPrivateProfileStringW(L"OneNote", L"RedirectURIPath", L"", RedirectURIPath, LB_SIZE, wIniFileName.c_str());

    GetPrivateProfileStringW(L"OneNote", L"EndpointHost", L"", EndpointHost, LB_SIZE, wIniFileName.c_str());
    GetPrivateProfileStringW(L"OneNote", L"OAuthEndpoint", L"", OAuthEndpoint, LB_SIZE, wIniFileName.c_str());

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
            [hWnd, RedirectURIHost, RedirectURIPath](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

                // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                env->CreateCoreWebView2Controller(hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hWnd, RedirectURIHost, RedirectURIPath](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
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
                            [hWnd, RedirectURIHost, RedirectURIPath](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                wil::unique_cotaskmem_string uri;
                                args->get_Uri(&uri);
                                std::wstring source(uri.get());

                                web::uri redirectURI(source);
                                if (redirectURI.host() == RedirectURIHost && redirectURI.path() == RedirectURIPath) {
                                    wchar_t* QueryString = (wchar_t*)malloc((redirectURI.query().size() + 1) * sizeof(wchar_t));
                                    if (QueryString)
                                    {
                                        wcscpy_s(QueryString, redirectURI.query().size() + 1, redirectURI.query().c_str());
                                        PostMessage(hWnd, WM_DONELOGINTOMS, NULL, (LPARAM)QueryString);
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

void OAuth::ResizeLogonWindow(HWND hWnd) {
    if (webviewController != nullptr) {
        RECT bounds;
        GetClientRect(hWnd, &bounds);
        bounds.right = bounds.left + 500;
        webviewController->put_Bounds(bounds);
    }
}

