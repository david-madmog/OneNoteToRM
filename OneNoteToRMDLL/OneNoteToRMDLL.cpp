// OneNoteToRM DLL 

#include "pch.h"
#include "OneNoteToRMDLL.h"

#include "OneNoteToRM.h"
#include "RMDocFile.h"
#include "WindowRMPage.h"
#include "ToOneRMPage.h"
#include "ToRMOnePage.h"
#include "WindowONEPage.h"
#include "GraphDoc.h"
#include "RMAPI.h"
#include <windowsx.h>

/*******************************************************************************

    OneNoteToRMDLL.cpp

    See header for documentation

    (C) David Poirier 2026

********************************************************************************/


///////////////////////////////////////////////////////////////////
// Global and utility functions - not exported

// Source - https://stackoverflow.com/a/18374698
// Retrieved 2026-03-17, License - CC BY-SA 4.0
#pragma warning(disable : 4996)

std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}
std::wstring s2ws(const std::unique_ptr<char> str) {
    return s2ws(str.get());
}
std::string ws2s(const std::unique_ptr<wchar_t> str) {
    return ws2s(str.get());
}


const char* LogLevelName[] = {
    "VERB",
    "DBUG",
    "INFO",
    "WARN",
    "*ERR"
};

const int LogLevelColour[] = { 8, 7, 10, 14, 12 };
HWND hLogListbox = NULL;

LogLevel CurrentLevel = LogLevel::LOG_INFO;
//LogLevel CurrentLevel = LogLevel::LOG_WARNING;
bool bConsoleMode = false;

void DoLog(const char* Class, const std::wstring& Msg, LogLevel Level)
{
    DoLog(Class, Msg.c_str(), Level);
}


void DoLog(const char* Class, const wchar_t* Msg, LogLevel Level)
{
    if (Level >= CurrentLevel)
    {
        std::wostringstream buff;
        std::time_t time = std::time({});
        char timeString[std::size("yyyy-mm-ddThh:mm:ssZ")];
        struct tm tmDest;
        localtime_s(&tmDest, &time);
        std::strftime(std::data(timeString), std::size(timeString), "%F %T", &tmDest);

#pragma warning( suppress : 33011)
        buff << L"[" << timeString << L"][" << LogLevelName[Level] << L"][" << Class << L"]:" << Msg;
        if (hLogListbox)
        {
            int NewItem = ListBox_AddString(hLogListbox, buff.str().c_str());
            ListBox_SetTopIndex(hLogListbox, NewItem);
        }

        buff << std::endl;
//        if (bConsoleMode)
//        {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            if (Level <= sizeof(LogLevelColour))
#pragma warning ( suppress : 6385 )
                SetConsoleTextAttribute(hConsole, LogLevelColour[Level]);
            std::cout << ws2s(buff.str());
//        }
//        else
            OutputDebugString(buff.str().c_str());
    }
}

void DoLog(const char* Class, const char* Msg, LogLevel Level)
{
    std::wstring WS;
    WS = s2ws(Msg);
    DoLog(Class, WS.c_str(), Level);
}


////////////////////////////////////////////////////
// Static storage
GraphAPI* gAPI = NULL;
RMAPI* rmAPI = NULL;
char gszIniFileName[] = ".\\OneNoteToRM.ini";

//wchar_t gszwIniFileName[] = L".\\OneNoteToRM.ini";

/////////////////////////////////////////////////////////////
// Main exported functions
LPSTR GetIniFile() {
    return gszIniFileName;
}

// Non-C++ languages have trouble marshalling a returned string, so alternative scheme is to copy string into a supplied buffer
HRESULT GetIniFileB(LPCSTR Buffer, int BuffLen) {
    int err = strcpy_s((char*)Buffer, (long)BuffLen, gszIniFileName);
    return err;
}

// If set, DLL logging will send a "ListBox_AddString" message to this list box 
void SetLogListbox(HWND hWnd)
{
    hLogListbox = hWnd;

    char LogLevelStr[100];
    GetPrivateProfileStringA("Settings", "LogLevel", "0", LogLevelStr, 99, gszIniFileName);

    CurrentLevel = static_cast<LogLevel>(std::atoi(LogLevelStr));
}

void DoLog(const char* Class, const char* Msg, int Level)
{
    std::wstring WS;
    WS = s2ws(Msg);
    DoLog(Class, WS.c_str(), static_cast<LogLevel>(Level));

}

// Used to set the oAuth login token for MS Graph API
void SetToken(const int PageType, LPCWSTR LoginCodeW)
{
    switch (PageType)
    {
    case PAGE_TYPE_WINDOW_RM_PAGE:
    case PAGE_TYPE_TO_ONE_RM_PAGE:
        if (!rmAPI)
            rmAPI = new RMAPI();
        rmAPI->SetAuthCode(LoginCodeW);
        break;
    case PAGE_TYPE_WINDOW_ONE_PAGE:
    case PAGE_TYPE_TO_RM_ONE_PAGE:
        if (!gAPI)
            gAPI = new GraphAPI();
        gAPI->SetAuthCode(LoginCodeW);
        break;
    default:
        ;
    }
}

// Return a list of the known docuements 
HRESULT ListDocs(const int PageType, LPCWSTR Buffer, int BuffLen)
{
    std::wstring List(L"");
    BaseAPI* API = nullptr;

    switch (PageType)
    {
    case PAGE_TYPE_WINDOW_RM_PAGE:
    case PAGE_TYPE_TO_ONE_RM_PAGE:
        if (!rmAPI)
            rmAPI = new RMAPI();
        API = rmAPI;
        break;
    case PAGE_TYPE_WINDOW_ONE_PAGE:
    case PAGE_TYPE_TO_RM_ONE_PAGE:
        if (!gAPI)
            gAPI = new GraphAPI();
        API = gAPI;
        break;
    default:
        ;
    }

    if (!API)
        return E_FAIL;

    if (!API->EnsureConnected())
        return E_FAIL;

    List = API->ListDocsString();
    int err = wcscpy_s((wchar_t *)Buffer, (long)BuffLen, List.c_str());
    return err;
}


HDOCFILE CreateEmptyDoc(const int PageType)
{
    HDOCFILE Doc = NULL;
    switch (PageType)
    {
    case PAGE_TYPE_WINDOW_RM_PAGE:
        if (!rmAPI)
            rmAPI = new RMAPI();
        Doc = new RMDocFile<WindowRMPage>(rmAPI);
        break;
    case PAGE_TYPE_TO_ONE_RM_PAGE :
        if (!rmAPI)
            rmAPI = new RMAPI();
        Doc = new RMDocFile<ToOneRMPage>(rmAPI);
        break;
    case PAGE_TYPE_WINDOW_ONE_PAGE :
        if (!gAPI)
            gAPI = new GraphAPI();
        Doc = new GraphDoc<WindowONEPage>(gAPI);
        break;
    case PAGE_TYPE_TO_RM_ONE_PAGE :
        if (!gAPI)
            gAPI = new GraphAPI();
        Doc = new GraphDoc<ToRMOnePage>(gAPI);
        break;
    default:
        Doc = NULL;
    }

    return Doc;
}

int LoadDoc(HDOCFILE Doc, const char* FileName)
{
    int Pages = 0;
    BaseDoc * baseDoc = (BaseDoc *)Doc;

    if (typeid(*baseDoc).hash_code() != typeid(RMDocFile<WindowRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(RMDocFile<ToOneRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<WindowONEPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<ToRMOnePage>).hash_code()
        )
      return ERR_INVLAID_DOC_TYPE;

    // So, see if we've got a single or compound filename
    std::string sFile(FileName);
    std::string Msg = "Starting Load ";

    std::string part1, part2;
    // SO, we assume two parts must be seperated by / - and that's not going to be valid as a part of an Hash
    size_t Found = sFile.find_last_of("/");
    bool bGotID = false;
    if (Found != std::string::npos)
    {
        // Got one - must be NB/part2
        part1 = sFile.substr(0, Found);
        part2 = sFile.substr(Found + 1);
        Msg.append("by Name: ");
        Msg.append(part1);
        Msg.append(" - ");
        Msg.append(part2);
    }
    else {
        // No "/" character - must be an Hash
        Msg.append("by ID: ");
        Msg.append(FileName);
        bGotID = true;
    }

    Msg.append("...");
    DoLog("DLL MAIN", Msg.c_str(), LOG_INFO);

    if (bGotID)
        Pages = baseDoc->LoadDoc(s2ws(sFile).c_str());
    else
        Pages = baseDoc->LoadDoc(part1, part2);

    return Pages;
}

HRESULT ConvertPage(HDOCFILE Source, HDOCFILE Dest, int Page)
{
    BaseDoc* baseDoc = (BaseDoc*)Source;
    if (typeid(*baseDoc).hash_code() != typeid(RMDocFile<WindowRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(RMDocFile<ToOneRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<WindowONEPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<ToRMOnePage>).hash_code()
        )
        return ERR_INVLAID_DOC_TYPE;

    // Window type, dest should be a DrawDetailsParams
    // Conversion type, dest should be a DOCFILE
    baseDoc->DrawPage(Dest, Page);

    return 0;
}

HRESULT ConvertPageB(HDOCFILE Source, DrawDetailsParams* DDP, int Page)
{
    BaseDoc* baseDoc = (BaseDoc*)Source;
    if (typeid(*baseDoc).hash_code() != typeid(RMDocFile<WindowRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(RMDocFile<ToOneRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<WindowONEPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<ToRMOnePage>).hash_code()
        )
        return ERR_INVLAID_DOC_TYPE;

    baseDoc->DrawPage(DDP, Page);

    return 0;
}

time_t GetDocDateTime(HDOCFILE Source)
{
    BaseDoc* baseDoc = (BaseDoc*)Source;
    if (typeid(*baseDoc).hash_code() != typeid(RMDocFile<WindowRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(RMDocFile<ToOneRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<WindowONEPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<ToRMOnePage>).hash_code()
        )
        return ERR_INVLAID_DOC_TYPE;

    time_t LETime = baseDoc->LastEditTime();
    return LETime;

//    time_t NowTime;
//    time(&NowTime);
//    return NowTime;
}


int SaveDoc(HDOCFILE Doc, const char* FileName)
{
    int Pages = 0;
    BaseDoc* baseDoc = (BaseDoc*)Doc;
    if (typeid(*baseDoc).hash_code() != typeid(RMDocFile<WindowRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(RMDocFile<ToOneRMPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<WindowONEPage>).hash_code()
        && typeid(*baseDoc).hash_code() != typeid(GraphDoc<ToRMOnePage>).hash_code()
        )
        return ERR_INVLAID_DOC_TYPE;


    // So, see if we've got an Hash, or two parts
    std::string sFile(FileName);
    std::string Msg = "Starting Save ";

    std::string part1, part2;
    // SO, we assume two parts must be seperated by / - and that's not going to be valid as a part of an Hash
    size_t Found = sFile.find_last_of("/");
    bool bGotID = false;
    if (Found != std::string::npos)
    {
        // Got one - must be NB/part2
        part1 = sFile.substr(0, Found);
        part2 = sFile.substr(Found + 1);
        Msg.append("by Name: ");
        Msg.append(part1);
        Msg.append(" - ");
        Msg.append(part2);
    }
    else {
        // No "/" character - must be an Hash
        Msg.append("by ID: ");
        Msg.append(FileName);
        bGotID = true;
    }

    Msg.append("...");
    DoLog("DLL MAIN", Msg.c_str(), LOG_INFO);

    if (bGotID)
        Pages = baseDoc->SaveDoc(s2ws(sFile).c_str());
    else
        Pages = baseDoc->SaveDoc(part1, part2);

    return Pages;
}

HRESULT DeleteDoc(HDOCFILE Doc)
{
    BaseDoc* baseClass = (BaseDoc*)Doc;
    delete baseClass;
    return 0;
}


/*
if (typeid(*baseDoc).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code())
{
}
if (typeid(*baseDoc).hash_code() == typeid(RMDocFile<ToOneRMPage>).hash_code())
{
}
if (typeid(*baseDoc).hash_code() == typeid(GraphDoc<WindowONEPage>).hash_code())
{
}
if (typeid(*baseDoc).hash_code() == typeid(GraphDoc<ToRMOnePage>).hash_code())
{
}
else
assert(false);
*/


