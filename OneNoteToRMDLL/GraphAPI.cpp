#include "pch.h"
#include "GraphAPI.h"
/*******************************************************************************

    GraphAPI.cpp

    See header for documentation

    (C) David Poirier 2026

********************************************************************************/


#pragma warning ( push )
#pragma warning( disable : 26439 26495)
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/filestream.h>
#include <cpprest/asyncrt_utils.h>

#include <ShlObj_core.h>

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


GraphAPI::GraphAPI()
{
    std::wstring wIniFileName = s2ws(gszIniFileName);

    TokenIniFileName = ws2s(AppLocalDirectory);
    TokenIniFileName.append(TOKEN_INI);

    // So, when we're instantiated, get the refresh token and clear the access token
    Refresh_Token = new char[IB_SIZE];
    GetPrivateProfileStringA("OneNote", "RefreshToken", "", Refresh_Token, IB_SIZE, TokenIniFileName.c_str());
    Token = NULL;

    GetPrivateProfileStringA("OneNote", "EntraAppID", "", EntraAppID, IB_SIZE, gszIniFileName);
    GetPrivateProfileStringW(L"OneNote", L"TenantID", L"", TenantID, IB_SIZE, wIniFileName.c_str());
    GetPrivateProfileStringA("OneNote", "RedirectURI", "", RedirectURI, IB_SIZE, gszIniFileName);

    GetPrivateProfileStringW(L"OneNote", L"EndpointRoot", L"", EndpointRoot, IB_SIZE, wIniFileName.c_str());
    GetPrivateProfileStringW(L"OneNote", L"OAuthTokenEndpoint", L"", OAuthTokenEndpoint, IB_SIZE, wIniFileName.c_str());

    GetPrivateProfileStringW(L"OneNote", L"GraphRoot", L"", GraphRoot, IB_SIZE, wIniFileName.c_str());
    GetPrivateProfileStringW(L"OneNote", L"GraphHost", L"", GraphHost, IB_SIZE, wIniFileName.c_str());
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

    uri_builder URI;
    URI.set_path(TenantID);
    URI.append_path(OAuthTokenEndpoint);

    //     utility::string_t s = URI.to_string();   // for debugging

    utf8string body;
    body.append("client_id="); body.append(EntraAppID);
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
                size_t Num;
                wcstombs_s(&Num, Refresh_Token, LB_SIZE, RT.c_str(), LB_SIZE);
                std::wostringstream LB;
                LB << "Got Refresh token : " << Refresh_Token;
                DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

                WritePrivateProfileStringA("OneNote", "RefreshToken", Refresh_Token, TokenIniFileName.c_str());
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


void GraphAPI::SetAuthCode(const wchar_t* LoginCodeW) {
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

void GraphAPI::ListSections(std::vector<ONE_Section>& Sections) {
    // Get full list of sections and get the Hash of the one which matches our input
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
                SectionDets.Hash = tmp.str();
            
                Sections.push_back(SectionDets);
            }
        }
    }
}

std::wstring GraphAPI::ListDocsString() {
    std::vector<ONE_Section> Sections;
    std::wstringstream Result(L"");
      
    ListSections(Sections);
    for (auto& Section : Sections) {
        Result << Section.Notebook << L" - " << Section.Section << L"|" << Section.Hash << std::endl;
    }
    return Result.str();
}

