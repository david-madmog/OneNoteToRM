#include "pch.h"
#include "RMAPI.h"
#include "OneNoteToRM.h"

/*******************************************************************************

    RMAPI.cpp

    V2 - Using ReMarkable API Directly
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

#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000

#include "SHA256.h"
#include "base64.hpp"

#pragma warning ( push )
#pragma warning( disable : 4005 26819)
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )

using namespace std;

using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

using njson = nlohmann::json;

constexpr auto CatalogCacheINI=L"\\RMCatalog.ini";

RMAPI::RMAPI()
{
    crc32c_init_sw(); // Build the CRC32 table

    wIniFileName = s2ws(gszIniFileName);
    TokenIniFileName = ws2s(AppLocalDirectory);
    TokenIniFileName.append(TOKEN_INI);
    CatalogCache = AppLocalDirectory;
    CatalogCache.append(CatalogCacheINI);

    // So, when we're instantiated, get the bearer token 
    DeviceToken = new char[IB_SIZE];
    GetPrivateProfileStringA("RMAPI", "BearerToken", "", DeviceToken, IB_SIZE, TokenIniFileName.c_str());

    GetPrivateProfileStringW(L"RMAPI", L"StorageRoot", L"", StorageRoot, IB_SIZE, wIniFileName.c_str());
    GetPrivateProfileStringW(L"RMAPI", L"StorageDataPath", L"", StorageDataPath, IB_SIZE, wIniFileName.c_str());
}

RMAPI::~RMAPI()
{
    delete[] DeviceToken;
    delete UserToken;
}

int RMAPI::RegisterDevice(const char * deviceCode) {
    std::wostringstream LB;
    LB << "Register Device Code : " << deviceCode;
    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

    // Build the payload request
    
    web::json::value RegisterPayload;
    RegisterPayload[L"code"] = json::value(s2ws(deviceCode));
    RegisterPayload[L"deviceDesc"] = json::value(L"desktop-windows");

    UUID uuid;
    if (UuidCreate(&uuid) != RPC_S_OK)
        return -1;
    wchar_t* str=NULL;
    if (UuidToStringW(&uuid, (RPC_WSTR*)&str) != RPC_S_OK)
        return -1;

    RegisterPayload[L"deviceID"] = json::value(str);
    RpcStringFreeA((RPC_CSTR*)&str);

    // Build request URI 
    wchar_t Data[IB_SIZE];
    uri_builder URI;
    GetPrivateProfileStringW(L"RMAPI", L"RegisterDevice", L"", Data, IB_SIZE, wIniFileName.c_str());
    URI.set_path(Data);

    utility::string_t s = URI.to_string();   // for debugging
    s = RegisterPayload.serialize();

    http_client_config config;
    config.set_timeout(utility::seconds(60));
    wchar_t TokenHostRoot[IB_SIZE];
    GetPrivateProfileStringW(L"RMAPI", L"TokenHostRoot", L"", TokenHostRoot, IB_SIZE, wIniFileName.c_str());

    http_client client(TokenHostRoot, config);

    http_request request(methods::POST);
    request.headers().add(L"authorization", L"Bearer");
    request.headers().add(L"accept-encoding", L"gzip");
    request.set_request_uri(URI.to_uri());
    request.set_body(RegisterPayload.serialize());
    utility::string_t ContentType(web::http::details::mime_types::application_octetstream);
    request.headers().set_content_type(ContentType);

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
        pplx::task<utility::string_t> RespDataTask = response.extract_string(true); /// need to handle multipart response - sets ignore content type to true
        utility::string_t* RespData = new utility::string_t(RespDataTask.get());
        std::wostringstream LB;
        LB << L"GOT data: " << RespData->substr(0, LB_SIZE - 50);
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);

        if (DeviceToken)
            delete[] DeviceToken;

        DeviceToken = new char[IB_SIZE];
        strncpy_s(DeviceToken, IB_SIZE - 1, ws2s(*RespData).c_str(), IB_SIZE - 1);
        WritePrivateProfileStringA("RMAPI", "DeviceToken", DeviceToken, TokenIniFileName.c_str());

        return 0;
    }
    else {
        std::wostringstream LB;
        LB << L"Cannot receive data: " << response.reason_phrase();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
    }
    return -1;

}

int RMAPI::GetUserToken(char * DeviceToken) {
    if (strlen(DeviceToken)) {
        //Simply send a POST request with the current device token as 'Authorization: Bearer ' header.
        //The response is the new token in plain text.
        std::wostringstream LB;
        LB << "Getting User Token from Device Token: " << DeviceToken;
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

        // Build request URI 
        wchar_t Data[IB_SIZE];
        uri_builder URI;
        GetPrivateProfileStringW(L"RMAPI", L"RefreshUser", L"", Data, IB_SIZE, wIniFileName.c_str());
        URI.set_path(Data);

        utility::string_t s = URI.to_string();   // for debugging

        http_client_config config;
        config.set_timeout(utility::seconds(60));
        wchar_t TokenHostRoot[IB_SIZE] ;
        GetPrivateProfileStringW(L"RMAPI", L"TokenHostRoot", L"", TokenHostRoot, IB_SIZE, wIniFileName.c_str());
        if (!wcslen(TokenHostRoot))
            return -1;

        http_client client(TokenHostRoot, config);   
        http_request request(methods::POST);

        wstring Token(L"Bearer ");
        Token.append(s2ws(DeviceToken));
        request.headers().add(L"authorization", Token);
        request.set_request_uri(URI.to_uri());

        request.headers().set_content_length(0);
        utility::string_t ContentType(web::http::details::mime_types::application_octetstream);
        request.headers().set_content_type(ContentType);
        request.headers().add(L"accept-encoding", L"gzip");

        pplx::task<http_response> requestTask = client.request(request);

        http_response response;
        try {
            response = requestTask.get(); // If task is not complete, will wait
        }
        catch (std::exception& ex)
        {
            std::wostringstream LB;
            LB << L"Cannot Refresh Token: " << ex.what();
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
            return -1;
        }

        if (response.status_code() == status_codes::OK)
        {
            pplx::task<utility::string_t> RespDataTask = response.extract_string(true); 
            utility::string_t* RespData = new utility::string_t(RespDataTask.get());
            std::wostringstream LB;
            LB << L"GOT data: " << RespData->substr(0, LB_SIZE - 50);
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);

            if (UserToken)
                delete UserToken;
            UserToken = new wstring(RespData->c_str());
            return 0;
        }
        else {
            std::wostringstream LB;
            LB << L"Cannot receive data: " << response.reason_phrase();
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
            return -1;
        }
        return 0;
    }
    else {
        // We shouldn't be in a position where we have a device code but no Device token, so just abort here
        return -1;
    }
}

bool RMAPI::EnsureConnected(void) {
    // So, do we have a user token?
    if (UserToken) {
        return true;
    }
    else {
        // We need to get one... do we have a device token?
        char* DeviceToken = new char[IB_SIZE];
        GetPrivateProfileStringA("RMAPI", "DeviceToken", "", DeviceToken, IB_SIZE, TokenIniFileName.c_str());
        if (strlen(DeviceToken))
        {
            // Yes, so we can log in (which will get a user token for future calls
            int iRet = GetUserToken(DeviceToken);
            if (iRet)
                return false;
            else
                return true;
        }
        else {
            // No Device token... do we have a device code (set from UI)?
            char* DeviceCode = new char[IB_SIZE];
            GetPrivateProfileStringA("RMAPI", "DeviceCode", "", DeviceCode, IB_SIZE, TokenIniFileName.c_str());
            if (strlen(DeviceCode))
            {
                // OK, must be first time, so get a token, and then log in
                int iRet = RegisterDevice(DeviceCode);
                if (iRet == 0)
                    int iRet = GetUserToken(DeviceToken);

                if (iRet)
                    return false;
                else
                    return true;

            }
            else {
                // No info in registry at all, ask our caller to get us a Device Code
                return false;
            }
        }
    }
}


void RMAPI::SetAuthCode(const wchar_t* DeviceCodeW) {

    std::string DeviceCode = ws2s(DeviceCodeW);
    std::wostringstream LB;
    LB << "Got Device Code : " << DeviceCodeW;
    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

    WritePrivateProfileStringA("RMAPI", "DeviceCode", DeviceCode.c_str(), TokenIniFileName.c_str());

}

std::wstring* RMAPI::GetDataStorage(const wchar_t* hash, const wchar_t* RMFilename)
{
    return GetStorage(StorageDataPath, hash, RMFilename);
}

wstring* RMAPI::GetStorage(const wchar_t* path, const wchar_t* hash, const wchar_t* RMFilename)
{
    if (UserToken) {
        // Build request URI 
        uri_builder URI;
        URI.set_path(path);
        if (hash)
            URI.append_path(hash);

        utility::string_t s = URI.to_string();   // for debugging

        http_client client(StorageRoot);
        http_request request(methods::GET);

        wstring Token(L"Bearer ");
        Token.append(*UserToken);
        request.headers().add(L"authorization", Token);
        request.set_request_uri(URI.to_uri());

        request.headers().set_content_length(0);
        utility::string_t ContentType(web::http::details::mime_types::application_octetstream);
        request.headers().set_content_type(ContentType);
        request.headers().add(L"accept-encoding", L"gzip");
        request.headers().add(L"rm-filename", RMFilename);

        pplx::task<http_response> requestTask = client.request(request);

        http_response response;
        try {
            response = requestTask.get(); // If task is not complete, will wait
        }
        catch (std::exception& ex)
        {
            std::wostringstream LB;
            LB << L"Cannot Load Storage Data: " << ex.what();
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
            return nullptr;
        }
        return nullptr;
    }
    else {
        return nullptr;
    }
}

concurrency::streams::istream RMAPI::GetPage(const wchar_t* hash, const wchar_t* RMFilename)
{
    if (UserToken) {
        // Build request URI 
        uri_builder URI;
        URI.set_path(StorageDataPath);
        if (hash)
            URI.append_path(hash);

        utility::string_t s = URI.to_string();   // for debugging

        http_client client(StorageRoot);
        http_request request(methods::GET);

        wstring Token(L"Bearer ");
        Token.append(*UserToken);
        request.headers().add(L"authorization", Token);
        request.set_request_uri(URI.to_uri());

        request.headers().set_content_length(0);
        utility::string_t ContentType(web::http::details::mime_types::application_octetstream);
        request.headers().set_content_type(ContentType);
        request.headers().add(L"accept-encoding", L"gzip");
        request.headers().add(L"rm-filename", RMFilename);

        pplx::task<http_response> requestTask = client.request(request);

        http_response response;
        try {
            response = requestTask.get(); // If task is not complete, will wait
        }
        catch (std::exception& ex)
        {
            std::wostringstream LB;
            LB << L"Cannot Load Storage Data: " << ex.what();
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);

            return concurrency::streams::istream();
        }

        if (response.status_code() == status_codes::OK)
        {
            concurrency::streams::istream iStream = response.body();
            std::wostringstream LB;
            LB << L"GOT data (stream)" ;
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
            return iStream; //!!!CHECK WE DELETE THIS
        }
        else {
            std::wostringstream LB;
            LB << L"Cannot receive data: " << response.reason_phrase();
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
            return concurrency::streams::istream();
        }
        return concurrency::streams::istream();
    }
    else {
        return concurrency::streams::istream();
    }
}

int64_t RMAPI::PutDataStorage(const wchar_t* hash, const wchar_t* RMFilename, const char* BodyData)
{
    size_t DataLen = strlen(BodyData);
    return PutStorage(StorageDataPath, hash, RMFilename, BodyData, DataLen);
}

int64_t RMAPI::PutDataStorage(const wchar_t* hash, const wchar_t* RMFilename, const char* BodyData, size_t DataLen) 
{
    return PutStorage(StorageDataPath, hash, RMFilename, BodyData, DataLen);
}

int64_t RMAPI::PutStorage(const wchar_t* path, const wchar_t* node, const wchar_t* RMFilename, const char* BodyData, size_t DataLen)
{
    if (UserToken) {
        // Build request URI 
        uri_builder URI;
        URI.set_path(path);
        if (node)
            URI.append_path(node);

        utility::string_t s = URI.to_string();   // for debugging

        http_client client(StorageRoot);
        http_request request(methods::PUT);

        wstring Token(L"Bearer ");
        Token.append(*UserToken);
        request.headers().add(L"authorization", Token);
        request.set_request_uri(URI.to_uri());

        //utility::string_t ContentType(web::http::details::mime_types::application_octetstream);
        //request.headers().set_content_type(ContentType);
        request.headers().add(L"accept-encoding", L"gzip");
        request.headers().add(L"rm-filename", RMFilename);

        wstring GH(L"crc32c=");
        uint32_t crc = crc32c_sw(0, BodyData, DataLen);
        char bytes[] = { (char)(crc >> 24), (char)((crc >> 16) & 0xFF), (char)((crc >> 8) & 0xFF), (char)(crc & 0xFF), 0 };
        string encoded = base64::to_base64(bytes);
        GH.append(s2ws(encoded));

        request.headers().add(L"x-goog-hash", GH.c_str());
        std::vector<unsigned char> BD(BodyData, BodyData + DataLen);
        request.set_body(BD);

        pplx::task<http_response> requestTask = client.request(request);

        http_response response;
        try {
            response = requestTask.get(); // If task is not complete, will wait
        }
        catch (std::exception& ex)
        {
            std::wostringstream LB;
            LB << L"Cannot put Data EX: " << ex.what() << L" (URI:" << URI.to_string() << L", RM Filename:" << RMFilename << L")";
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);

            return 0;
        }

        if (response.status_code() == status_codes::OK 
            || response.status_code() == status_codes::Accepted
            )
        {
            std::wostringstream LB;
            LB << L"PUT storage doc (" << RMFilename << L")";
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

            return request.headers().content_length();
        }
        else {
            std::wostringstream LB;
            LB << L"Cannot put data: " << response.reason_phrase() << L" (URI:" << URI.to_string() << L", RM Filename:" << RMFilename << L")";;
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
            return 0;
        }
    }
    return 0;
}


std::wstring RMAPI::ListDocsString() {
    DoLog(typeid(*this).name(), "Querying RM API for doc list", LOG_DEBUG);
    unordered_map<wstring, DocNode> Nodes;

    // So, first call the doc root
    wchar_t Data[IB_SIZE];
    GetPrivateProfileStringW(L"RMAPI", L"StorageRootPath", L"", Data, IB_SIZE, wIniFileName.c_str());

    wstring* RootData = GetStorage(Data, nullptr, L"");
    if (!RootData)
        return L"";

    njson respJson;
    try {
        respJson = njson::parse(*RootData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return L"";
    }

    if (respJson.contains("hash")) {
        // Yay, found our root hash
        //"hash": "de5d158da3f264c5bb339f22bf7e995625314c82f54dc898ab7130ab3ec31601",
        wstring H = s2ws(respJson["hash"]);
        LoadRootNodes(Nodes, H);
    }

    // Now, tie up the nodes' parents
    wostringstream Ret;
    for (auto& it : Nodes)
    {
        DocNode N = it.second;
        if (N.Type == NodeType::FileData)
        {
            if (N.Parent.empty())
                N.Path = N.UnitName;
            else if (N.Parent == L"trash")
                N.Path = L"trash" + wstring(Sep) + N.UnitName;
            else
                N.Path = RecursePath(Nodes, N);
            Ret << N << endl;
        }
    }
    return Ret.str();
}

wstring RMAPI::RecursePath(unordered_map<wstring, DocNode>& Nodes, DocNode Node)
{
    if (Node.Parent.empty()) {
        return Node.UnitName;
    } else {
        return RecursePath(Nodes, Nodes[Node.Parent]) + Sep + Node.UnitName;
    }

}

void RMAPI::LoadRootNodes(unordered_map<wstring, DocNode>& Nodes, wstring& Hash)
{
    wstring NodeUUID(L"root.docSchema");
    wstring* NodeData = GetStorage(StorageDataPath, Hash.c_str(), NodeUUID.c_str());
    if (!NodeData)
        return;

    // So, first split into lines
    // e.g. 250240707f951ddd6c859b9a163556d55837115f22d1e52fc997ca7abf329d39:0:0071ec9e-8eef-4ea6-b3e5-21260a0d5458:1:250
    std::wstringstream AllNodes(*NodeData);
    wstring line;
    while (getline(AllNodes, line))
    {
        // Split into sections seperated by ":"
        vector<wstring> Segments;
        std::wstringstream Segs(line);
        wstring Seg;
        while (getline(Segs, Seg, L':'))
            Segments.push_back(Seg);

        // The first field is the Hash we want to query, the third id the UUID
        if (Segments.size() < 5)
            continue;
        
        DocNode NewNode;
        wstring Hash = Segments[0];
        NewNode.Hash = Hash;
        NewNode.UUID = Segments[2];
 
        // See if it's changed since last time we looked
        bool FoundInCache = NewNode.LoadFromCache(NewNode.UUID, CatalogCache.c_str());

        if (NewNode.Hash != Hash || !FoundInCache)
        {
            NewNode.Hash = Hash;
            LoadFileData(&NewNode);
            std::wostringstream LB;
            LB << L"Loaded from API: " << NewNode.UnitName ;
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

            NewNode.SaveToCache(CatalogCache.c_str());
        }
        else {
            std::wostringstream LB;
            LB << L"Loaded from Cache: " << NewNode.UnitName ;
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);
        }

        Nodes[NewNode.UUID] = NewNode;
    }
}

void RMAPI::LoadFileData(DocNode* Node)
{
    wstring NodeUUID = Node->UUID;
    NodeUUID.append(L".docSchema");
    wstring* NodeData = GetStorage(StorageDataPath, Node->Hash.c_str(), NodeUUID.c_str());
    if (!NodeData)
        return;

    // So, first split into lines
// e.g. c81c44980d2a67b170cab01e8a617e14571d8cfa8015bc9b3c454ddb369e50bb:0:bd924c5d-1e0c-40af-a915-f1931a1c9524.content:0:59659
    std::wstringstream ss(*NodeData);
    wstring line;

    while (getline(ss, line))
    {
        // See if it's a "metadata" line - that's what we want
        if (line.find(L"metadata") != string::npos)
        {
            // Split into sections seperated by ":"
            vector<wstring> Segments;
            std::wstringstream Segs(line);
            wstring Seg;
            while (getline(Segs, Seg, L':'))
                Segments.push_back(Seg);

            // The first field is the Hash we want to query, the third id the UUID
            if (Segments.size() < 5)
                continue;

            // The first field is the Hash we want to query, the third id the UUID
            wstring Hash = Segments[0];
            wstring UUID = Segments[2];
            LoadMetadata(Hash, UUID, Node);
        }
    }

}

void RMAPI::LoadMetadata(wstring& NodeID, wstring& NodeUUID, DocNode* Node)
{ 
    wstring* NodeData = GetStorage(StorageDataPath, NodeID.c_str(), NodeUUID.c_str());
    if (!NodeData)
        return;

    // this should be a Json doc... and contains the actual name we want!
    njson respJson;
    try {
        respJson = njson::parse(*NodeData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return;
    }

    if (respJson.contains("type"))
    {
        if (respJson["type"].get<string>() == "DocumentType")
            Node->Type = NodeType::FileData;
        else
            Node->Type = NodeType::Directory;
        if (respJson.contains("parent"))
            Node->Parent = s2ws(respJson["parent"].get<string>());
        if (respJson.contains("visibleName"))
            Node->UnitName = s2ws(respJson["visibleName"].get<string>());

    }

}
/*
* **********************************************
* Emergency code in case we trash the root schema
* 
void RMAPI::RecoverRootDocSchema() {
    // Find and Load the previous Root Doc Schema
// So, first call the doc root
    wchar_t D[IB_SIZE];
    GetPrivateProfileStringW(L"RMAPI", L"StorageRootPath", L"", D, IB_SIZE, wIniFileName.c_str());

    wstring* RootData = GetStorage(D, nullptr, L"");
    njson respJson;
    try {
        respJson = njson::parse(*RootData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return;
    }

    if (!respJson.contains("hash"))
        return;

    // Prepare the new DocSchema
    wostringstream DocSchema(L"");

    wchar_t Data[LB_SIZE];
    wchar_t* P = Data;
    GetPrivateProfileStringW(NULL, NULL, L"", Data, IB_SIZE, CatalogCache.c_str());
    vector<wstring> IDs;
    size_t Sz;
    do {
        wstring S(P);
        IDs.push_back(S);
        Sz = S.size();
        P += Sz + 1;
    } while ( Sz > 0 );

    // Split into lines
    // e.g. 250240707f951ddd6c859b9a163556d55837115f22d1e52fc997ca7abf329d39:0:0071ec9e-8eef-4ea6-b3e5-21260a0d5458:1:250
    for (wstring ID: IDs)
    { 
        DocNode UpdatedNode;
        UpdatedNode.LoadFromCache(ID, CatalogCache.c_str());
            // NOTE: requires the entries to be sorted by doc hash. Since this is unchanged, we can update in place
        DocSchema << UpdatedNode.Hash << L":0:" << UpdatedNode.UUID << L":" << 1 << L":" << UpdatedNode.Len;
        DocSchema << "\n";
    }

    wostringstream DocSchemaFull(L"");
    DocSchemaFull << L"3\n";
    DocSchemaFull << L"0:.:" << IDs.size() << ":" << DocSchema.str().size() << L"\n";
    DocSchemaFull << DocSchema.str();

     // And post the new root schema back to the server
    string Content = ws2s(DocSchemaFull.str());
    string NewHash = sha256(Content);
 //   PutDataStorage(s2ws(NewHash).c_str(), L"root.docSchema", Content.c_str());

    // Since the root schema hash has changed, we need to update that as well
    respJson["hash"] = "fc92216cc7c69c65b5a3a0580f8f8d43ae300b23e9be9ba8fbe2b7274809cb32";
    respJson["broadcast"] = false;
    //  int64_t gen = respJson["generation"];
    //  respJson["generation"] = ++gen;

    Content = respJson.dump();

    GetPrivateProfileStringW(L"RMAPI", L"StorageRootPutPath", L"", Data, IB_SIZE, wIniFileName.c_str());
    PutStorage(Data, nullptr, L"roothash", Content.c_str(), Content.size());
    /*
    {"broadcast":false,"hash":"44ac407c979f4ca9a0ce5b0c5b3a85cc826c664c3103a6a634865dda4429cf22","generation":1787579158542117}
    
    return;

}
*/
void RMAPI::UpdateRootDocSchema(DocNode& UpdatedNode, int NumPages) 
{
    // Update the cache
    UpdatedNode.SaveToCache(CatalogCache.c_str());

    // Find and Load the previous Root Doc Schema
    // So, first call the doc root
    wchar_t Data[IB_SIZE];
    GetPrivateProfileStringW(L"RMAPI", L"StorageRootPath", L"", Data, IB_SIZE, wIniFileName.c_str());

    wstring* RootData = GetStorage(Data, nullptr, L"");
    njson respJson;
    try {
        respJson = njson::parse(*RootData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return ;
    }

    if (!respJson.contains("hash"))
        return ;

    wstring Hash = s2ws(respJson["hash"]);
    wstring NodeUUID(L"root.docSchema");
    wstring* NodeData = GetStorage(StorageDataPath, Hash.c_str(), NodeUUID.c_str());
    if (!NodeData)
        return ;

    // Prepare the new root DocSchema based on the old one
    // Note, unlike the v3 docschema used for the actual doc, the v4 docschema uses the simple hash
    wostringstream DocSchema(L"");

    // Split into lines
    // e.g. 250240707f951ddd6c859b9a163556d55837115f22d1e52fc997ca7abf329d39:0:0071ec9e-8eef-4ea6-b3e5-21260a0d5458:1:250
    std::wstringstream AllNodes(*NodeData);
    int Lines = 0;
    long long TotalSize = 0;

    wstring line;
    while (getline(AllNodes, line))
    {
        // Split into sections seperated by ":"
        vector<wstring> Segments;
        std::wstringstream Segs(line);
        wstring Seg;
        while (getline(Segs, Seg, L':'))
            Segments.push_back(Seg);

        // If it's not the updated node, just copy through unchanged
        if (Segments.size() < 5) // v4 schema header
        {
            if (Segments.size() > 1) 
            {
                // v4 index line... must be at the start, before all the doc entries
                // Note, we're not changing the number of entries
                //0:.:86:620150930        0, ., Count of entries, size of file
                Lines = stoi(Segments[2]);
            }
        }
        else {
            if (Segments[2] != UpdatedNode.UUID)
            {
                DocSchema << line;
            }
            else {
                // Updated node, regenerate line
                // NOTE: requires the entries to be sorted by doc hash. Since this is unchanged, we can update in place
                DocSchema << UpdatedNode.Hash << L":0:" << UpdatedNode.UUID << L":" << NumPages << L":" << UpdatedNode.Len;
                Segments[0] = UpdatedNode.Hash;
                Segments[4] = std::to_wstring(UpdatedNode.Len);
            }
            TotalSize += stol(Segments[4]);
            DocSchema << "\n";
        }
    }

    // And post the new root schema back to the server
    std::stringstream ContentSS("");
    ContentSS << "4\n";
    ContentSS << "0:.:" << Lines << ":" << TotalSize << "\n";
    ContentSS << ws2s(DocSchema.str());
    std::string NewHash = sha256(ContentSS.str());
    int64_t ret = PutDataStorage(s2ws(NewHash).c_str(), L"root.docSchema", ContentSS.str().c_str());

    if (ret == 0)
        return;

    // Since the root schema hash has changed, we need to update that as well
    respJson["hash"] = NewHash;
    respJson["broadcast"] = false;

    string Content = respJson.dump();

    GetPrivateProfileStringW(L"RMAPI", L"StorageRootPutPath", L"", Data, IB_SIZE, wIniFileName.c_str());
    PutStorage(Data, nullptr, L"roothash", Content.c_str(), Content.size());
    /*
    {"broadcast":false,"hash":"44ac407c979f4ca9a0ce5b0c5b3a85cc826c664c3103a6a634865dda4429cf22","generation":1787579158542117}
    */
    return;
}

//////////////////////////////////////////////////////////////////

void RMAPI::DocNode::SaveToCache(const wchar_t * CatalogCache) const
{
    WritePrivateProfileStringW(UUID.c_str(), L"Hash", Hash.c_str(), CatalogCache);
    WritePrivateProfileStringW(UUID.c_str(), L"Parent", Parent.c_str(), CatalogCache);
    WritePrivateProfileStringW(UUID.c_str(), L"UnitName", UnitName.c_str(), CatalogCache);
    switch (Type)
    {
    case NodeType::Directory:
        WritePrivateProfileStringW(UUID.c_str(), L"Type", L"D", CatalogCache);
        break;
    case NodeType::FileData:
        WritePrivateProfileStringW(UUID.c_str(), L"Type", L"F", CatalogCache);
        break;
    default:
        WritePrivateProfileStringW(UUID.c_str(), L"Type", L"?", CatalogCache);
        break;
    }
}

bool RMAPI::DocNode::LoadFromCache(std::wstring sID, const wchar_t* CatalogCache)
{
    UUID = sID;
    wchar_t Buff[IB_SIZE];
    GetPrivateProfileStringW(UUID.c_str(), L"Hash", L"", Buff, IB_SIZE, CatalogCache);
    if (wcsnlen(Buff, IB_SIZE) == 0) // Entry was not found, so default has been returned...
        return false;
    Hash = wstring(Buff);
    GetPrivateProfileStringW(UUID.c_str(), L"Parent", L"", Buff, IB_SIZE, CatalogCache);
    Parent = wstring(Buff);
    GetPrivateProfileStringW(UUID.c_str(), L"UnitName", L"", Buff, IB_SIZE, CatalogCache);
    UnitName = wstring(Buff);
    GetPrivateProfileStringW(UUID.c_str(), L"Type", L"", Buff, IB_SIZE, CatalogCache);
    switch (Buff[0])
    {
    case 'D':
        Type = NodeType::Directory;
        break;
    case 'F':
        Type = NodeType::FileData;
        break;
    default:
        Type = NodeType::Unknown;
        break;
    }
    return true;
}

void RMAPI::DocNode::DeleteFromCache(const wchar_t* CatalogCache) const
{
    WritePrivateProfileStringW(UUID.c_str(), NULL, NULL, CatalogCache);
}

    /*
https://internal.cloud.remarkable.com/sync/v4/root
-->
{
    "hash": "de5d158da3f264c5bb339f22bf7e995625314c82f54dc898ab7130ab3ec31601",
    "generation": 1787138100167293,
    "schemaVersion": 3
}

                                      sync/v3/files/de5d158da3f264c5bb339f22bf7e995625314c82f54dc898ab7130ab3ec31601
https://internal.cloud.remarkable.com/sync/v3/files/de5d158da3f264c5bb339f22bf7e995625314c82f54dc898ab7130ab3ec31601
Header: rm-filename: root.docschema

-->
4                       Schema Version
0:.:86:620150930        0, ., Count of entries, size of file
250240707f951ddd6c859b9a163556d55837115f22d1e52fc997ca7abf329d39:0:0071ec9e-8eef-4ea6-b3e5-21260a0d5458:1:250
                        Hash, Type, Doc ID, Subfiles(pages):size
ae75ab0446349bf12e9bfbbbfd64cbb2082e5ff807c8039e63beabe4f7d7f80b:0:04f496fa-f4e1-407a-870c-e77a96302a21:3:154907
4be3c6efea8cd90a8548ddb276d1a2fafc8c40bfe28783cd1688abb76c62b3a5:0:0594dd88-ed32-45c7-82f9-570b1d5a8d51:4:43253651
1135a2c7199d477e8b5a0a349bfefc0c8b7db2d9fbd89132b2dc36e9e29aacf7:0:0640b6ff-a7d8-463d-bdea-5a9c1e451f68:6:754320
e30ba04b0ffb5a29642e37c285302333c61237ac116b516839f6025b128f260c:0:0841a5b4-70b7-4508-bd78-5ba693713ef0:5:5271448
39ce43d0e97a37ffba2018e50f18679d53af434b8593ff60bb6125bb818a233d:0:092dad21-ebe4-46f4-9ee0-a958515f3028:4:560047
6ecf68bd606157b4ae70b793c1d128852ed675390d714338f07b6ff925829732:0:0a515f08-0aa7-4892-b0d9-096edf2c19d6:7:1029460
307fce757441aec8460d6f25381025dad4281c15a047cb6fc90d2e7a91513943:0:0bb5dc8c-6d95-4c5d-a274-0cfbc9053de3:5:56577
86e7d777238baf34004216d145781316dcb9199b15b10c64b5683f8e2bbf8bd3:0:0c9c4649-93e1-4f89-96a7-1f04203ab33c:12:3058615
e929b45e89a5576cba122ca53be6074e55cd780741130b535b1444a84326c0e8:0:0d02c3b2-d612-437d-b401-42560c740a21:5:123510
af6ca03f8b4f25a7319b3a6fc31df14fe5f589c4a7ea7d0a0d62c6761c597b6e:0:0dc156a9-27c6-4a73-bf8c-5d2320c0aa7f:4:11015
f6bf70a77420825d90b3559ad49821df6c1b00185224ae0c54c645c209e92fce:0:12e83537-89e2-4f85-8a43-cbd54b5d79ac:7:364115
49d6bc827d89c7a0fe18097338cb61d7a21d2e36efd5cb5b5c2bcdf005de2863:0:17771899-beb4-42ac-9557-333e6555e3dc:5:144938
ee35fd3000aac4a1ece5c822d94dae8755e32c7a6666c9a8b477bce48b2c746c:0:21031937-1e63-4089-bb48-a1cc4dbdc3a7:5:7535586
14233770b0e4528056d827ddc14f9a3ff09a1e2d6f3523fd2ed829cb88bb4832:0:2322f1ca-f69b-4bc6-aa1f-0607cda69afc:5:3340052
e969630b852e969f7bbde553820faa62245b36e9c4100dfb1a75fb6627aa30f7:0:25240e6b-339d-49b2-93e4-5cc93e62cc5c:3:103811
afb85a0a237d0a8dbb46e181cfb195d5209049ef23267e181c8a1ad54088f3ea:0:2596b084-b504-42c2-bd60-42af234cf4ca:4:116953
3cbf546ca561b886448cde65a6aea58ab88fba1f1ca6c1f8a1670d9af0917cae:0:287b093c-8b78-4c72-9ec9-35ed9f840987:5:124255
e9a6b4344dc0cf70f442bd93f8a9f898e1c1bf07271637b7c7c1d72e1979533d:0:297bea72-690b-44e4-91a2-c1e595ce5c46:19:3682835
4946d4ac5ee67ab85ad1857b9c7b8e7b7b53820ce91146c02c85cb8bbfed26d5:0:29cc9dc0-96de-48e0-8032-7bb292a1d707:14:6835821
c5fc5abbf648176453611060c5229c1098e15241c02a54c04eaf835f4650d612:0:2a3b51f2-c5db-4ab6-ad27-3c1eaf15a438:5:122320
a38e19469e851777e78e9c8001f90c130c0b3796f4994d8a803243c123df2a16:0:3321fb0d-c2a7-4ad6-85ba-4c63426f243a:4:17466869
245edf071eb5599c158a5110d7031ccc4e7997b6b6adeb76f5c9d78363bfe5ce:0:333c47fd-d5d9-4e55-be59-1b2eb45eac19:4:89944642
f084f07e37ea7f1feaa1aef8f90a05cc5df39f27e3fb4fbda1fde58b97b77e06:0:34e7be24-02c2-4f36-b019-b6b9e8ba24bb:2:258
6f7f8611fc075917356860b27b7d8fb731827ed2d334845cb81bf46ba02c4915:0:36fc1d44-1e7f-41bf-8c73-16556da99177:4:82888988
8a9b73b4344d8cef8adab15c8184319067492fbb064d9ac040c8f01ca3c6d04e:0:37b13a36-880e-4398-b456-79dafb32bb24:5:285776
61512ec55d0598c0050ac7d19a79cb19739311928a19bdfec4cde9928601c522:0:37d43dc7-965e-4d1d-a4f9-efc1ace4d760:4:2007852
5ae5ff7f7e8e9dc7fe6c599c144d70b7b98ac336c830016feff32a9227f5d4ff:0:39374e0d-cd39-41ae-9fde-b5c7acfc3241:4:2062739
2e9fb520914ac1b8de68f6bc4df827e812ed339290e5580a220b059d10889776:0:4109f7e5-76cf-4a75-a4ce-9623aa12f37e:6:6385651
b190fcd703bc698541fc3c36c311489eaf6bb8cc8dc020f6af4ceb95b5af050c:0:49ba8407-a067-4daf-b8e1-cc5d7fc9f3ba:5:2555872
34fcfb410d6db0410ad2eb45f28eef48c3513a83b646363d075fcd2c526eb950:0:50200e6d-b1dd-4f83-9e29-c535379dffad:5:45353152
743e760d8fe485da7e40e59b2fc71f67bb78a191b363e010d6d1f168faeb965e:0:5546dee4-5733-4d79-be1e-e5821f8c0876:4:84086
932e58c18fbea056866924d6ca07ee64d40afdd2bae2ee8076db725a50d98995:0:559b32e9-aadd-4ade-b135-66ef16f077b2:5:65539
91a7495833dd2d5e804de6f802e39e1731ccdabb221497f07b7539826f62554c:0:637c1ee3-9152-4008-8247-3f8559ad1268:5:1654797
4584331852054782532cd120513bab5eb93b0bba3d3a2b13a22f0fbe1575ed03:0:63958c11-0e1c-4049-8a46-aeb8742751ff:4:85893
235049d95f1ba71502a53dfad322dd3077ec6c02c84170860e0df2a4cf0ad86c:0:6503df20-c429-4266-bd95-99d19b545a13:8:563488
b4c2fdaad3c1e72d88adcbe9a92206c11187add8b0526ddadb3bdd6c41b7ade1:0:66117378-c89c-4bdd-bd04-9a4fc44debe0:5:424222
3984514cee849b1840b4d2fe9ac666492a1653b0ba77027d2654b1cf5b9d6423:0:6672cdad-ea6a-4059-8bc5-c0ccb1d1fa62:5:4199718
f35f856e9d4268e6c606b4fb76bed1e01027c03ad57198e87b765c243b12ebfc:0:6794ac3a-adae-4cc4-a131-376b4c24dd05:5:2614857
2a2bd1c05b814e2706ae9026878d1332c0490e706f896561605431419e8dd244:0:69c42784-87d0-439b-ae3b-6f517ea9517b:5:625727
459b0282879a9903d9e489b0b3f7e46a5e4736d6540159c05d83395e848692aa:0:6a3b68ce-3210-42e3-a3e4-c85bc5217f06:5:226307
0ea0a9c9d15dce2684b5838b4a7445811d671c73ba9bbd26a53aebdfa003dbab:0:6fcf049c-2fda-4945-9cea-d53c62249985:5:2628366
f7ad6ebb20e5743036b26cb2f9a673714e1ba86c1e9d1cc4c4b029eccd8259d5:0:70de8fd3-9553-4c0b-9557-2ddb0f93accc:5:3995822
aff054dd3fb885e86883982fde760ee370314f0f91fc5b1f71d43350db8d4c43:0:722193bf-5c94-42cf-b10e-1218f9c78c1e:5:252811
9a4a9874e499f94dd038fc7127561bff8bad3c473dea7b1fa8b047779ab19172:0:74fcf69d-2d61-47fd-857e-5cf86a1e66cd:4:31052648
d364a62264d0b0d44cb863b97d4afa091bc8f28a11e2a6c2aca5c9642a21cf50:0:75b91ff4-5708-4f6a-b0fd-3c87a4961b50:5:3076204
ed5b952bc235bd13b8a787b6f86e2e170a693fdc6be1205c36c4f82ff7363d7a:0:760ecb59-25a4-4b9b-854e-fd28f2029d3f:5:126021
503f3a55ce188684ba595785be06f3251b939ee4df6e4ee9dcdb6464473c294d:0:7643cc3e-feff-4d22-a634-7ba2826f755f:5:4714851
995e9f154bce6e2831d85de4a706eabcc40baca86e8c29f217f885b205187b32:0:78baf584-2b43-41b9-859f-037b31eef0e7:4:13147
30dc14e5b4fbca496fed26a92f508fe9f943796d8c8f7219d28b79a61e607c8a:0:79fd0b95-c5c7-4f95-89a8-8ee7a7cb5c80:5:7015528
805bd74ea4aa4a9590eae73b7e2f36ab95d4fd1733a00648b5350367ce84079c:0:7a880efc-8e85-44cb-a002-faef60e54a7f:19:1812050
9f425e4930466dbd6577a91c388c09e7a5a51d5b196dd14b2f5a89ea5ed57a5a:0:7c3ed323-fd48-4887-9030-d468cc373cce:5:1492700
50b0ad3a58396fff8040a040fb1579fcb0a29b86337c2ec09c5abd387ec7e615:0:892dd2b4-2496-4897-aec9-5405f96fc766:2:255
7c7d82422af6a63c276084885a1c6b7863500392c957017e90fea04337763e2b:0:8d8f9859-ddc1-4b7d-97a4-c2395e58ca7b:5:128471
c37e5a76c760c6ffa8b3148dbf70e4b2ffa6b3dc211a3d1a16f502f4b0c4384d:0:8ec50e23-2f68-4b26-8450-6b0068786115:28:1001485
25a6b436f496930b4e6d7b31fbeb3b3e444ef7ad83efdb0fd8fe5f4737a5a9f0:0:94053033-ffae-478a-942a-74f5b26d9b0c:5:3826377
25c5cd91f3346952d99cd088c735860cef9ac3385751dee9bb7e0ff71858eaf0:0:94ebbd48-23ba-4f24-8b81-30b2f635d2e7:5:230452
1d522998f9e3f9d669dae1389cbdec5661314e6f2b705ae18c2d212cba585edc:0:99d90982-66ea-4178-9fff-1d160d2ac666:3:10671
8bf1111a44c8be87236773f2283893383956c15345643942455f8b88fc571725:0:a209c45f-f41f-4870-9773-64069baa09ed:5:2005960
510072b24b86d0d4cf7207876d4d9655869b7e965bfaf2081c81dd9209a568ee:0:a30408f9-1de4-4e43-a19a-d151b366cfc7:5:121479
c00d109e2ff61dc40f73f9a790af83f48bbb6ad905c1eea1538b0c53991adc73:0:a4989198-946c-427f-b26e-313d2a0cf955:5:6294301
0ad647523705bc4b091d5e8f087a342f1b4f9191f25fd514cf35ed0208a023d4:0:a5c8adc7-d2ce-4ab7-8a0a-fdcd2b05d49b:5:5185731
59fda21b1fc09a659cd8882f8e6fd97cea81456c71b19c132b59bf5d69e94b35:0:a5fc1824-0e22-4a19-87c1-7b513f2cb931:5:122064
39f64b9516bbb43638336d412bf1c4dfa3586717628ba06d43f5ee9b67630ff2:0:a7bc47a4-1155-4879-854e-942744aa1cbe:21:2680286
0e5dbcbc75d37bfc004741e0e5cd76a5a5f46f5c806e73ff734ee50a944bcaea:0:a7eee409-df7c-43ae-9d0b-bf75ef14622e:5:15287116
b1a0fdec623ef18823947bc65ef03c5281561db54c1cb27985dfe0cf79fdcc8d:0:b17fbaf7-2dbf-45ea-bdc3-aaf918e0becb:3:4582
4cbe8488a73fcd28f52fed7dd86ba311a3b755ebc4ed654e9e50536aab66a111:0:b653bb8c-636d-4cb2-8f8e-1b322829bf2b:4:87755
01f1c2e5def48bf902e364d17e0fa89c62da714a4313efb7d13cba851fd73ffb:0:bd924c5d-1e0c-40af-a915-f1931a1c9524:8:2378126
9928b2bbe7f67e18a27c718900da046c0e3c107ac6b7be33603e5208f5c8ada4:0:bf5173d6-ddcf-4cc8-bbb3-802e0400b6c7:10:997368
d02d3b361f6b858c4deabe2b8c8c68226e6b1782b11655bf76fb1e3204066dce:0:c2bd4a5b-ba29-4365-a6d8-dd0730564306:4:15672496
8711b7d90fc712485338223740b3be477df8cd6268f565b8dd8f3e99d4cd0cb6:0:c5807a1c-ca29-4ff8-af3c-ff26518527d0:99:31976795
2f735b88a260ff356d523fd53c980b37f28479a84f02cb56448f425eea3119b9:0:c77f8f5f-17a6-4ffe-87ff-055dacb57c40:5:224149
382e9edca34f9a0833a5bee09985db6dc4d6fbaebd4ba733e52742fa3619d048:0:cb5ae06f-e23d-410a-840c-3d3362e3bb72:4:84972
4addc93c77fea8b4e5cde38bcd3a25c4c3062bbf39f5128ebe80b6d875f989ec:0:ce674dd4-7785-496c-aa23-5c9090a00229:9:1242052
7cf936b94448a9fbadba7856125b73c2beaa560f0ec623c19d4c62d56b8cc540:0:d79f1565-7179-496d-870c-1d5b4fc677fa:5:6429453
6f86731c8e55242d495cb81994eb537cd52b4e3204a4edca10c679cfbd72e941:0:d8704efd-593a-4f2d-90a3-280aaab99f83:2:281
72a5f5e572e283d0ddd7e75e7fa9bdcaade86d1fb7501b1f9fddda9cd4a01e3f:0:d91422f6-d8e5-4d16-ae83-d5d73f66a8a1:3:54845
20ef5d86d00009dd7208d1dc62e64bde1481ba8f3f4fc89a6ca73b4c85ffa36b:0:ed341a75-68b8-49e5-a544-c7e9b8bab810:3:44321
bb6cd34c50081561ca39c5515868da22051e62e72cb522cb18caeb20a444280b:0:ee6639f5-e54a-44c1-88b5-26aa11a73736:5:215466
e3bc3834f57d0d309398561217dd55367adc6b40377a0f3157745bcab85e0d29:0:efc958ea-5564-476d-b598-4ca0c57ffefe:4:177966
efd2438c6b76b502867b840e25b01ae17aa54a09e2b1e0002b41036a48ea7c5f:0:f0a821dd-87af-4910-9297-7c7028b1bd01:270:92869437
413e4a1b5b68d48d9814ea2c5a5d401a9f6d390c06b4873ad6ac38bdbd9809ce:0:f2f04d44-68f1-4524-b2b1-0420cb29c3f3:4:59926
b204f8a49277850f1e11f8548d3f47ed1219ece1ef763bef3013c1675631ca14:0:f3d9c5ab-6df6-4fa7-8963-4f8b2888e893:6:118863
3652cde05ba5baf7b913ac460739035694d46a7f7dc141c5a2d6ac606b14ec53:0:f59e9c58-736c-49ff-b1e6-87320b13f8f6:2:279
c5e7663569d0eae39157b2073c6138fe5f2bf4509d06246f81307c8c3946e89c:0:fc45a602-1aba-49c1-8fab-7b495b33d3cf:4:34436871
315ca9da3c2dc6db4a1be88c0716025d8d7882d68506e0124a93c2a2bae3c127:0:ff56291b-1cde-455d-bd59-bd5357878248:5:3859212


https://internal.cloud.remarkable.com/sync/v3/files/01f1c2e5def48bf902e364d17e0fa89c62da714a4313efb7d13cba851fd73ffb
rm-filename: bd924c5d-1e0c-40af-a915-f1931a1c9524.docschema

-->
3
c81c44980d2a67b170cab01e8a617e14571d8cfa8015bc9b3c454ddb369e50bb:0:bd924c5d-1e0c-40af-a915-f1931a1c9524.content:0:59659
e2d67bb8cb07d480771974e18d280c6d651fae85df4f66c8d4dc58352e30371e:0:bd924c5d-1e0c-40af-a915-f1931a1c9524.metadata:0:375
7edbdb92a2490f7b2fabb7876734b1379a23f0e7908ee5f017a7cb389199e282:0:bd924c5d-1e0c-40af-a915-f1931a1c9524.pagedata:0:50
7c7fe9c13f45b3c857aa96f1fb9f6f4728dacb1612e13f850bfe2c9ba669a69f:0:bd924c5d-1e0c-40af-a915-f1931a1c9524.textconversion/f2fd65db-6521-4332-88d6-7007a5f9886b.json:0:170
a721c5d8495a0bb9d4826c1800e7899b979f3a8d528d5cc64434faf9c21552e8:0:bd924c5d-1e0c-40af-a915-f1931a1c9524/107f8519-b789-4282-a5b5-d2f8a930fb28-metadata.json:0:76
d07da7db815ceddf8500d31234179c1938683bed411c04500cd91a00783336a9:0:bd924c5d-1e0c-40af-a915-f1931a1c9524/107f8519-b789-4282-a5b5-d2f8a930fb28.rm:0:1653852
e642ff0d72b5224ef905ec6a81be8714e3319f916fd6013350392559bb7ce53f:0:bd924c5d-1e0c-40af-a915-f1931a1c9524/6849484e-fe89-4917-b93e-96a38f0d0874.rm:0:35449
17717afea9d6e62c63115857b3783ab90ec5ca64d584c68bb440d60690f9b8ba:0:bd924c5d-1e0c-40af-a915-f1931a1c9524/84563da9-7f9b-445b-9249-e8187ce1025f.rm:0:628495


https://internal.cloud.remarkable.com/sync/v3/files/c81c44980d2a67b170cab01e8a617e14571d8cfa8015bc9b3c454ddb369e50bb
-->
{
...
}

https://internal.cloud.remarkable.com/sync/v3/files/e2d67bb8cb07d480771974e18d280c6d651fae85df4f66c8d4dc58352e30371e
-->
{
    "createdTime": "0",
    "deleted": false,
    "lastModified": "1787138099448",
    "lastOpened": "1786969584877",
    "lastOpenedPage": 2,
    "metadatamodified": true,
    "modified": true,
    "new": false,
    "parent": "",
    "pinned": true,
    "source": "",
    "synced": true,
    "type": "DocumentType",
    "version": 272,
    "visibleName": "Quick sheets"
}


*/



