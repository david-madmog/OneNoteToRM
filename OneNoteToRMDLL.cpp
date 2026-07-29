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

//const LogLevel CurrentLevel = LogLevel::LOG_INFO;
LogLevel CurrentLevel = LogLevel::LOG_WARNING;
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
char gszIniFileName[] = ".\\OneNoteToRM.ini";
char gszTokenFileName[] = ".\\OneNoteToRMToken.ini";

//wchar_t gszwIniFileName[] = L".\\OneNoteToRM.ini";

/////////////////////////////////////////////////////////////
// Main exported functions
LPSTR GetIniFile() {
    return gszIniFileName;
}

HRESULT GetIniFileB(LPCSTR Buffer, int BuffLen) {
    int err = strcpy_s((char*)Buffer, (long)BuffLen, gszIniFileName);
    return err;
}

void SetLogListbox(HWND hWnd)
{
    hLogListbox = hWnd;

    char LogLevelStr[100];
    GetPrivateProfileStringA("Settings", "LogLevel", "0", LogLevelStr, 99, gszIniFileName);

    CurrentLevel = static_cast<LogLevel>(std::atoi(LogLevelStr));
}

void SetToken(const int PageType, LPCWSTR LoginCodeW)
{
    switch (PageType)
    {
    case PAGE_TYPE_WINDOW_RM_PAGE:
    case PAGE_TYPE_TO_ONE_RM_PAGE:
        // Not really used at this time;
        break;
    case PAGE_TYPE_WINDOW_ONE_PAGE:
    case PAGE_TYPE_TO_RM_ONE_PAGE:
        if (!gAPI)
            gAPI = new GraphAPI();
        gAPI->SetLoginCode(LoginCodeW);
        break;
    default:
        ;
    }
}

HRESULT ListDocs(const int PageType, LPCSTR Buffer, int BuffLen)
{
    std::string List("");

    switch (PageType)
    {
    case PAGE_TYPE_WINDOW_RM_PAGE:
    case PAGE_TYPE_TO_ONE_RM_PAGE:
        List = RMAPI::ListDocsString();
        break;
    case PAGE_TYPE_WINDOW_ONE_PAGE:
    case PAGE_TYPE_TO_RM_ONE_PAGE:
        if (!gAPI)
            gAPI = new GraphAPI();
        if (!gAPI->EnsureConnected())
            return E_FAIL;

        List = gAPI->ListDocsString();
        break;
    default:
        ;
    }

    int err = strcpy_s((char *)Buffer, (long)BuffLen, List.c_str());
    return err;
}


HDOCFILE CreateEmptyDoc(const int PageType)
{
    HDOCFILE Doc = NULL;
    switch (PageType)
    {
    case PAGE_TYPE_WINDOW_RM_PAGE:
        Doc = new RMDocFile<WindowRMPage>();
        break;
    case PAGE_TYPE_TO_ONE_RM_PAGE :
        Doc = new RMDocFile<ToOneRMPage>();
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
    Drawable * baseClass = nullptr;
    baseClass = (Drawable *)Doc;

    if (typeid(*baseClass).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code()
        || typeid(*baseClass).hash_code() == typeid(RMDocFile<ToOneRMPage>).hash_code())
    {
        std::string sRMFile(FileName);
        std::string Msg = "Starting RM Load : ";
        Msg.append(sRMFile);
        Msg.append("...");
        DoLog("MAIN", Msg.c_str(), LOG_INFO);

        RMAPI::GetDoc(sRMFile);

        size_t Found = sRMFile.find_last_of("\\");
        if (Found != std::string::npos)
            sRMFile = sRMFile.substr(Found + 1);

        std::unique_ptr<char> WorkingDir(new char[LB_SIZE]);
        GetTempPathA(LB_SIZE - 1, WorkingDir.get());
        std::string Zipfile = WorkingDir.get();
        Zipfile.append(sRMFile);
        Zipfile.append(".rmdoc");

        if (typeid(*baseClass).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code())
        {
            RMDocFile<WindowRMPage>* TypeDoc = (RMDocFile<WindowRMPage> *)Doc;
            Pages = TypeDoc->ExtractRMsFromZip(Zipfile.c_str());
        }
        else {
            RMDocFile<ToOneRMPage>* TypeDoc = (RMDocFile<ToOneRMPage> *)Doc;
            Pages = TypeDoc->ExtractRMsFromZip(Zipfile.c_str());
        }
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<WindowONEPage>).hash_code())
    {
        GraphDoc<WindowONEPage>* TypeDoc = (GraphDoc<WindowONEPage> *)Doc;
        Pages = TypeDoc->LoadPages(s2ws(FileName).c_str());
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<ToRMOnePage>).hash_code())
    {
        GraphDoc<ToRMOnePage>* TypeDoc = (GraphDoc<ToRMOnePage> *)Doc;
        Pages = TypeDoc->LoadPages(s2ws(FileName).c_str());
    }
    else
        Pages = ERR_INVLAID_DOC_TYPE;

    return Pages;
}

HRESULT ConvertPage(HDOCFILE Source, HDOCFILE Dest, int Page)
{
    int Pages = 0;
    Drawable* baseClass = nullptr;
    baseClass = (Drawable*)Source;

    if (typeid(*baseClass).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code())
    {
        // Window type, dest should be a DrawDetailsParams
        RMDocFile<ToOneRMPage>* TypeDoc = (RMDocFile<ToOneRMPage> *)Source;
        TypeDoc->DrawPage(Dest, Page);
    }
    else if (typeid(*baseClass).hash_code() == typeid(RMDocFile<ToOneRMPage>).hash_code())
    {
        // Conversion type, dest should be a DOCFILE
        RMDocFile<ToOneRMPage>* TypeDoc = (RMDocFile<ToOneRMPage> *)Source;
        TypeDoc->DrawPage(Dest, Page);
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<WindowONEPage>).hash_code())
    {
        // Window type, dest should be a DrawDetailsParams
        GraphDoc<WindowONEPage>* TypeDoc = (GraphDoc<WindowONEPage> *)Source;
        TypeDoc->DrawPage(Dest, Page);
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<ToRMOnePage>).hash_code())
    {
        // Conversion type, dest should be a DOCFILE
        GraphDoc<ToRMOnePage>* TypeDoc = (GraphDoc<ToRMOnePage> *)Source;
        TypeDoc->DrawPage(Dest, Page);
    }
    else
        assert(false);
    return 0;
}

HRESULT ConvertPageB(HDOCFILE Source, DrawDetailsParams* DDP, int Page)
{
    return ConvertPage(Source, (HDOCFILE)DDP, Page);
}

time_t GetDocDateTime(HDOCFILE Source)
{
    Drawable* baseClass = nullptr;
    baseClass = (Drawable*)Source;
    time_t LETime;

    if (typeid(*baseClass).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code())
    {
        RMDocFile<ToOneRMPage>* TypeDoc = (RMDocFile<ToOneRMPage> *)Source;
        LETime = TypeDoc->LastEditTime();
    }
    else if (typeid(*baseClass).hash_code() == typeid(RMDocFile<ToOneRMPage>).hash_code())
    {
        RMDocFile<ToOneRMPage>* TypeDoc = (RMDocFile<ToOneRMPage> *)Source;
        LETime = TypeDoc->LastEditTime();
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<WindowONEPage>).hash_code())
    {
        GraphDoc<WindowONEPage>* TypeDoc = (GraphDoc<WindowONEPage> *)Source;
        LETime = TypeDoc->LastEditTime();
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<ToRMOnePage>).hash_code())
    {
        GraphDoc<ToRMOnePage>* TypeDoc = (GraphDoc<ToRMOnePage> *)Source;
        LETime = TypeDoc->LastEditTime();
    }
    else
        assert(false);
    return LETime;

    time_t NowTime;
    time(&NowTime);
    return NowTime;
}


int SaveDoc(HDOCFILE Doc, const char* FileName)
{
    int Pages = 0;
    Drawable* baseClass = nullptr;
    baseClass = (Drawable*)Doc;

    if (typeid(*baseClass).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code()
        || typeid(*baseClass).hash_code() == typeid(RMDocFile<ToOneRMPage>).hash_code())
    {
        std::unique_ptr<char> WorkingDir(new char[LB_SIZE]);
        GetTempPathA(LB_SIZE - 1, WorkingDir.get());

        std::wstring Msg = L"Starting RM Save: ";
        Msg.append(s2ws(FileName));
        Msg.append(L"...");
        DoLog("DLL MAIN", Msg, LOG_INFO);
        std::string TMPfile = WorkingDir.get();
        TMPfile.append("Output.rmdoc");

        if (typeid(*baseClass).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code())
        {
            RMDocFile<WindowRMPage>* TypeDoc = (RMDocFile<WindowRMPage> *)Doc;
            TypeDoc->Name = FileName;
            Pages = TypeDoc->SaveRMsToZip(TMPfile.c_str());
            RMAPI::SaveDoc(FileName, WorkingDir.get());
        }
        else {
            RMDocFile<ToOneRMPage>* TypeDoc = (RMDocFile<ToOneRMPage> *)Doc;
            TypeDoc->Name = FileName;
            Pages = TypeDoc->SaveRMsToZip(TMPfile.c_str());
            RMAPI::SaveDoc(FileName, WorkingDir.get());
        }
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<WindowONEPage>).hash_code())
    {
        GraphDoc<WindowONEPage>* TypeDoc = (GraphDoc<WindowONEPage> *)Doc;
        std::wstring Msg = L"Starting ONE Save: ";
        Msg.append(s2ws(FileName));
        Msg.append(L"...");
        DoLog("DLL MAIN", Msg, LOG_INFO);
        Pages = TypeDoc->SaveDoc(s2ws(FileName).c_str());
    }
    else if (typeid(*baseClass).hash_code() == typeid(GraphDoc<ToRMOnePage>).hash_code())
    {
        GraphDoc<ToRMOnePage>* TypeDoc = (GraphDoc<ToRMOnePage> *)Doc;
        std::wstring Msg = L"Starting ONE Save: ";
        Msg.append(s2ws(FileName));
        Msg.append(L"...");
        DoLog("DLL MAIN", Msg, LOG_INFO);
        Pages = TypeDoc->SaveDoc(s2ws(FileName).c_str());
    }
    else
        Pages = ERR_INVLAID_DOC_TYPE;

    return Pages;
}



/*
if (typeid(*baseClass).hash_code() == typeid(RMDocFile<WindowRMPage>).hash_code())
{
}
if (typeid(*baseClass).hash_code() == typeid(RMDocFile<ToOneRMPage>).hash_code())
{
}
if (typeid(*baseClass).hash_code() == typeid(GraphDoc<WindowONEPage>).hash_code())
{
}
if (typeid(*baseClass).hash_code() == typeid(GraphDoc<ToRMOnePage>).hash_code())
{
}
else
assert(false);
*/


