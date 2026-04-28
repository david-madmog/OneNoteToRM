// DOCXToRM.cpp : Defines the entry point for the application.
//

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "framework.h"
#include "OneNoteToRM.h"
#include "RMDocFile.h"
#include "WindowRMPage.h"
#include "ToOneRMPage.h"
#include "ToRMOnePage.h"
#include "WindowONEPage.h"
#include "GraphDoc.h"
#include "RMAPI.h"
#include <windowsx.h>
#include "resource.h"
#include "zip.h"

#include <thread> 

using namespace Gdiplus;

constexpr auto MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR szPopupWindowClass[MAX_LOADSTRING];           
WCHAR szPreviewWindowClass[MAX_LOADSTRING];          

RMDocFile<WindowRMPage>* ZF;
RMDocFile<ToOneRMPage>* TOZF;
GraphDoc<WindowONEPage>* GD;
GraphDoc<ToRMOnePage>* TOGD;
GraphAPI* gAPI;
int NumPages;
int CurrentPage;
HWND hLoginPopup;
HWND hPreview;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
ATOM                MyRegisterPopupClass(HINSTANCE hInstance);
ATOM                MyRegisterPreviewClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PopupWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PreviewWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


struct ONE_ID {
    bool Empty = true;
    std::wstring FileID;
    std::string Notebook;
    std::string Section;
};

std::string RM_Name_FromList(HWND hWnd);
ONE_ID OneID_FromList(HWND hWnd);
void RM_Preview(HWND hWnd);
void ONE_Preview(HWND hWnd);
void RM_to_ONE(std::string RMFile, ONE_ID OneID, bool Notebook);
void ONE_to_RM(std::string RMFile, ONE_ID OneID, bool Notebook);
void Timed(std::string RMFile, ONE_ID OneID );



int ProcessCommandLine(std::wstring CommandLine);
std::wstring ParseCommandLine(std::wstring Flag, std::wstring CommandLine);

// Window controls... create and lay out
HWND hRMList;
HWND hONEList;
HWND hListBox = 0;
HWND hImage;

HWND RMPreview;
HWND ONEPreview;
HWND hRMToOne;
HWND hONEToRM;
HWND hRMToOneNotebook;
HWND hONEToRMNotebook;
HWND hTimed;
HWND RMRefresh;
HWND ONERefresh;

HWND RM1;
HWND RM3;
HWND ON1;

std::thread * WorkerThread;

// Source - https://stackoverflow.com/a/18374698
// Retrieved 2026-03-17, License - CC BY-SA 4.0
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


char gszIniFileName[] = ".\\OneNoteToRM.ini";

char * LogBuffer = new char[LB_SIZE];

const char* LogLevelName[] = {
    "VERB",
    "DBUG",
    "INFO",
    "WARN",
    "*ERR"
};

const int LogLevelColour[] = { 8, 7, 10, 14, 12 };

const LogLevel CurrentLevel = LogLevel::LOG_INFO;
LogLevel ConsoleLevel = LogLevel::LOG_DEBUG;
bool bConsoleMode = false;

void DoLog(const char* Class, const std::wstring& Msg, LogLevel Level)
{
    DoLog(Class, Msg.c_str(), Level);
}


void DoLog(const char* Class, const wchar_t* Msg, LogLevel Level)
{
    if (Level >= ConsoleLevel)
    {
        std::wostringstream buff;
        std::time_t time = std::time({});
        char timeString[std::size("yyyy-mm-ddThh:mm:ssZ")];
        struct tm tmDest;
        localtime_s(&tmDest, &time);
        std::strftime(std::data(timeString), std::size(timeString), "%F %T", &tmDest);

        if (Level <= LOG_ERROR)
        {
            buff << L"[" << timeString << L"][" << LogLevelName[Level] << L"][" << Class << L"]:" << Msg;
            if (Level >= CurrentLevel && hListBox)
            {
                int NewItem = ListBox_AddString(hListBox, buff.str().c_str());
                ListBox_SetTopIndex(hListBox, NewItem);
            }

            buff << std::endl;
            if (bConsoleMode)
            {
                HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTextAttribute(hConsole, LogLevelColour[Level]);
                std::cout << ws2s(buff.str());
            }
            else
                OutputDebugString(buff.str().c_str());
        }
    }
}

void DoLog(const char * Class, const char* Msg, LogLevel Level)
{
    std::wstring WS;
    WS = s2ws(Msg);
    DoLog(Class, WS.c_str(), Level);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    // See if we're in silent/command line mode
    std::wstring CommandLine(lpCmdLine);
    if (CommandLine.find(L"-M") != std::string::npos || CommandLine.find(L"-m") != std::string::npos)
    {
        bConsoleMode = true;
        return ProcessCommandLine(CommandLine);
    }
    else
    {
        GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR           gdiplusToken;

        // Initialize GDI+.
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        // Initialize global strings
        LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
        LoadStringW(hInstance, IDC_DOCXTORM, szWindowClass, MAX_LOADSTRING);
        LoadStringW(hInstance, IDC_DOCXTORML, szPopupWindowClass, MAX_LOADSTRING);
        LoadStringW(hInstance, IDC_DOCXTORMP, szPreviewWindowClass, MAX_LOADSTRING);
        MyRegisterClass(hInstance);
        MyRegisterPopupClass(hInstance);
        MyRegisterPreviewClass(hInstance);

        // Perform application initialization:
        if (!InitInstance(hInstance, nCmdShow))
        {
            return FALSE;
        }

        HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DOCXTORM));

        MSG msg;

        // Main message loop:
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        GdiplusShutdown(gdiplusToken);

        return (int)msg.wParam;
    }
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DOCXTORM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = 0;
    //    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DOCXTORM);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

ATOM MyRegisterPopupClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PopupWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DOCXTORM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = szPopupWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

ATOM MyRegisterPreviewClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PreviewWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DOCXTORM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = szPreviewWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void LayoutWindow(HWND hWnd)
{
    RECT ClientArea;

    GetClientRect(hWnd, &ClientArea);

    long ColW = (ClientArea.right - ClientArea.left) / 9;
    long hPad = ColW / 10;

    long RowH = (ClientArea.bottom - ClientArea.top) / 5;

    SetWindowPos(hRMList, NULL, hPad, hPad, (4 * ColW - hPad), (3 * RowH - hPad), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(hONEList, NULL, (5 * ColW + hPad), hPad, (4 * ColW - hPad), (3 * RowH - hPad), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(hRMToOne, NULL, (9 * ColW) / 2 + hPad - 40, RowH + hPad - 35, 80, 35, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(hONEToRM, NULL, (9 * ColW) / 2 + hPad - 40, RowH + 2 * hPad, 80, 35, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(hRMToOneNotebook, NULL, (9 * ColW) / 2 + hPad - 40, RowH + 3 * hPad + 35, 80, 35, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(hONEToRMNotebook, NULL, (9 * ColW) / 2 + hPad - 40, RowH + 4 * hPad + 70, 80, 35, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(hTimed, NULL, (9 * ColW) / 2 + hPad - 40, RowH + 5 * hPad + 105, 80, 35, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(RMPreview, NULL, hPad, (3 * RowH + hPad), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(RMRefresh, NULL, 2 * hPad + 120, (3 * RowH + hPad), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    SetWindowPos(ONEPreview, NULL, (5 * ColW + hPad), (3 * RowH + hPad), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(ONERefresh, NULL, (5 * ColW + 2 * hPad + 120), (3 * RowH + hPad), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

#ifdef SAVEBUTTON
    SetWindowPos(RM1, NULL, hPad, (3 * RowH + 2 * hPad + 30), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
//    SetWindowPos(RM3, NULL, 3 * hPad + 240, (3 * RowH + 2 * hPad + 30), 150, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(ON1, NULL, (5 * ColW + hPad), (3 * RowH + 2 * hPad + 30), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    SetWindowPos(hListBox, NULL, hPad, (3 * RowH + 3 * hPad + 60), (9 * ColW - hPad), (ClientArea.bottom - (3 * RowH + 4 * hPad + 60)), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#else
    SetWindowPos(hListBox, NULL, hPad, (3 * RowH + 2 * hPad + 30), (9 * ColW - hPad), (ClientArea.bottom - (3 * RowH + 3 * hPad + 30)), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#endif
}

void LoadControls(HWND hWnd) {
    hRMList = CreateWindowEx(WS_EX_CLIENTEDGE, _T("listbox"),
        _T("caption.c_str()"),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        10, 120, 500, 500,
        hWnd, (HMENU)LST_LISTBOX,
        hInst, 0);

    hONEList = CreateWindowEx(WS_EX_CLIENTEDGE, _T("listbox"),
        _T("caption.c_str()"),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        10, 120, 500, 500,
        hWnd, (HMENU)LST_LISTBOX_ONE,
        hInst, 0);

    RMPreview = CreateWindow(_T("button"), _T("Preview"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        10, 10, 120, 30,
        hWnd, (HMENU)BTN_RM_PREVIEW_BUTTON, GetModuleHandle(NULL), NULL);
    RMRefresh = CreateWindow(_T("button"), _T("Refresh"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        250, 10, 120, 30,
        hWnd, (HMENU)BTN_RM_REFRESH, GetModuleHandle(NULL), NULL);

#ifdef SAVEBUTTON
    RM1 = CreateWindow(_T("button"), _T("Save RMDOC"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        130, 10, 120, 30,
        hWnd, (HMENU)BTN_BUTTONSAVE, GetModuleHandle(NULL), NULL);
    //RM3 = CreateWindow(_T("button"), _T("Download/Upload"),
    //    WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
    //    370, 10, 150, 30,
    //    hWnd, (HMENU)CHK_RELOAD, GetModuleHandle(NULL), NULL);
#endif

    hRMToOne = CreateWindow(_T("button"), _T("Overwrite\r→"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_MULTILINE,
        55, 45, 120, 50,
        hWnd, (HMENU)BTN_RM_TO_ONE, GetModuleHandle(NULL), NULL);

    hONEToRM = CreateWindow(_T("button"), _T("Overwrite\r←"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_MULTILINE,
        175, 45, 120, 50,
        hWnd, (HMENU)BTN_ONE_TO_RM, GetModuleHandle(NULL), NULL);

    hRMToOneNotebook = CreateWindow(_T("button"), _T("Use Name\r→"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_MULTILINE,
        55, 45, 120, 50,
        hWnd, (HMENU)BTN_RM_TO_ONE_NB, GetModuleHandle(NULL), NULL);

    hONEToRMNotebook = CreateWindow(_T("button"), _T("Use Name\r←"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_MULTILINE,
        175, 45, 120, 50,
        hWnd, (HMENU)BTN_ONE_TO_RM_NB, GetModuleHandle(NULL), NULL);

    hTimed = CreateWindow(_T("button"), _T("← ? →"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_MULTILINE,
        175, 45, 120, 50,
        hWnd, (HMENU)BTN_TIMED, GetModuleHandle(NULL), NULL);



    ONEPreview = CreateWindow(_T("button"), _T("Preview"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        10, 80, 120, 30,
        hWnd, (HMENU)BTN_ONE_PREVIEW_BUTTON, GetModuleHandle(NULL), NULL);

    ONERefresh = CreateWindow(_T("button"), _T("Refresh"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        130, 80, 120, 30,
        hWnd, (HMENU)BTN_ONE_REFRESH, GetModuleHandle(NULL), NULL);
#ifdef SAVEBUTTON
    ON1 = CreateWindow(_T("button"), _T("Save ONE doc"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        130, 80, 120, 30,
        hWnd, (HMENU)BTN_BUTTON_ONESAVE, GetModuleHandle(NULL), NULL);
#endif

    hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, _T("listbox"),
        _T("caption.c_str()"),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        10, 120, 500, 500,
        hWnd, (HMENU)LST_LISTBOX,
        hInst, 0);
    ListBox_SetHorizontalExtent(hListBox, 1000);

    LayoutWindow(hWnd);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        LoadControls(hWnd);
        PostMessage(hWnd, WM_LOADRMDOCS, NULL, NULL);
        PostMessage(hWnd, WM_LOADONEDOCS, NULL, NULL);
    break;    
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case BTN_RM_PREVIEW_BUTTON:
                RM_Preview(hWnd);
                break;
            case BTN_RM_REFRESH:
                PostMessage(hWnd, WM_LOADRMDOCS, NULL, NULL);
                break;
#ifdef SAVEBUTTON
            case BTN_BUTTONSAVE:
            {
                std::unique_ptr<wchar_t> RMFile(new wchar_t[LB_SIZE]);
                int CurSel = ListBox_GetCurSel(hRMList);
                ListBox_GetText(hRMList, CurSel, RMFile.get());
                std::string sRMFile = ws2s(RMFile.get());

                std::unique_ptr<char> WorkingDir(new char[LB_SIZE]);
                GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir.get(), LB_SIZE, gszIniFileName);

                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting  RM SAVE..."));

                std::string TMPfile = WorkingDir.get();
                TMPfile.append("Output.rmdoc");
                if (ZF)
                    NumPages = ZF->SaveRMsToZip(TMPfile.c_str());

                RMAPI::SaveDoc(sRMFile, WorkingDir.get());
            }
                break;
#endif
            case BTN_ONE_PREVIEW_BUTTON:
                ONE_Preview(hWnd);
    			break;
            case BTN_ONE_REFRESH:
                PostMessage(hWnd, WM_LOADONEDOCS, NULL, NULL);
                break;
            case BTN_RM_TO_ONE:
            {
                DoLog("MAIN", L"Starting RM to ONE...(Section overwrite)", LOG_INFO);
                std::string RMFile = RM_Name_FromList(hWnd);
                ONE_ID OneID = OneID_FromList(hWnd);
                if (!OneID.Empty && !RMFile.empty())
                {
                    if (TOZF)
                    {
                        delete TOZF;
                        TOZF = nullptr;
                    }
                    WorkerThread = new std::thread(RM_to_ONE, RMFile, OneID, false);
                }
            }
                break;
            case BTN_ONE_TO_RM:
            {
                DoLog("MAIN", L"Starting ONE to RM...(File Overwrite)", LOG_INFO);
                std::string RMFile = RM_Name_FromList(hWnd);
                ONE_ID OneID = OneID_FromList(hWnd);
                if (!OneID.Empty && !RMFile.empty())
                {
                    if (TOZF)
                    {
                        delete TOZF;
                        TOZF = nullptr;
                    }
                    WorkerThread = new std::thread(ONE_to_RM, RMFile, OneID, false);
                }
            }
                break;
            case BTN_RM_TO_ONE_NB:
            {
                DoLog("MAIN", L"Starting RM to ONE...", LOG_INFO);
                std::string RMFile = RM_Name_FromList(hWnd);
                ONE_ID OneID = OneID_FromList(hWnd);
                if (!OneID.Empty && !RMFile.empty())
                {
                    if (TOGD)
                    {
                        delete TOGD;
                        TOGD = nullptr;
                    }
                    WorkerThread = new std::thread(RM_to_ONE, RMFile, OneID, true);
                }
            }
                break;
            case BTN_ONE_TO_RM_NB:
            {
                DoLog("MAIN", L"Starting ONE to RM...", LOG_INFO);
                std::string RMFile = ""; // We don't need the name in this mode
                ONE_ID OneID = OneID_FromList(hWnd);
                if (!OneID.Empty)
                {
                    if (TOGD)
                    {
                        delete TOGD;
                        TOGD = nullptr;
                    }
                    WorkerThread = new std::thread(ONE_to_RM, RMFile, OneID, true);
                }
            }
            break;
            case BTN_TIMED:
            {
                DoLog("MAIN", L"Starting Time Comparison update...", LOG_INFO);
                std::string RMFile = RM_Name_FromList(hWnd);
                ONE_ID OneID = OneID_FromList(hWnd);
                if (!OneID.Empty && !RMFile.empty())
                    WorkerThread = new std::thread(Timed, RMFile, OneID);
            }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_LOADRMDOCS:
    {
        ListBox_ResetContent(hRMList);
        std::vector<std::wstring> Docs;
        RMAPI::ListDocs(Docs);
        for (auto& Doc : Docs) {
            ListBox_AddString(hRMList, Doc.c_str());
        }
    }
        break;
    case WM_LOADONEDOCS:
    {
        ListBox_ResetContent(hONEList);
        std::vector<ONE_Section> Sections;
        std::vector<std::wstring> Notebooks;
        if (!gAPI)
            gAPI = new GraphAPI();
        if (!gAPI->EnsureConnected())
        {
            DoLog("MAIN", L"No Auth, starting popup", LOG_INFO);
            hLoginPopup = CreateWindowW(szPopupWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                CW_USEDEFAULT, 0, 500, 650, hWnd, nullptr, hInst, nullptr);

            if (!hLoginPopup) {
                DWORD err = GetLastError();
                std::wostringstream LB;
                LB << L"Creating Popup Error: 0x" << std::hex << std::uppercase << err;
                DoLog("MAIN", LB.str(), LOG_ERROR);
            }
            else {
                ShowWindow(hLoginPopup, SW_NORMAL);
                UpdateWindow(hLoginPopup);
                PostMessage(hLoginPopup, WM_LOGINTOMS, NULL, (LPARAM)gAPI);
            }
        }

        gAPI->ListSections(Sections);
        for (auto& Section : Sections) {
            std::wstring Name = Section.Notebook;
            Notebooks.push_back(Name);
            Name.append(L" - ");
            Name.append(Section.Section);
            LRESULT Index = ListBox_AddString(hONEList, Name.c_str());
            wchar_t * SID(new wchar_t[1023]);
            wcscpy_s(SID, 1023, Section.ID.c_str());
            ListBox_SetItemData(hONEList, Index, SID);
            //delete[] SID;   We need to delete this memory when we empty the list box, it's needed until then
        }
    }
        break;
    case WM_SIZE:
        LayoutWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for popup login to microsoft  box.
LRESULT CALLBACK PopupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_CREATE:
        DoLog("POPUP", "WM_CREATE", LOG_DEBUG_VERBOSE);
        break;
    case WM_NCCREATE:
        DoLog("POPUP", "WM_NCCREATE", LOG_DEBUG_VERBOSE);
        break;
    case WM_LOGINTOMS:
        DoLog("POPUP", "WM_LOGINTOMS", LOG_DEBUG_VERBOSE);
        if (!gAPI)
            gAPI = new GraphAPI();
        gAPI->LoginToMicrosoft(hWnd);
        SetTimer(hWnd, WM_LOGINTOMS, 3000, (TIMERPROC)NULL);
        break;
    case WM_TIMER:
        DoLog("POPUP", "WM_TIMER", LOG_DEBUG_VERBOSE);
        KillTimer(hWnd, WM_LOGINTOMS);
        if (gAPI)
            gAPI->ResizeLogonWindow(hWnd);
        break;
    case WM_DONELOGINTOMS:
        DoLog("POPUP", "WM_DONELOGINTOMS", LOG_DEBUG_VERBOSE);
        gAPI->SetLoginCode((wchar_t*)lParam);
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        SetForegroundWindow(GetParent(hWnd));
        break;
    case WM_SIZE:
        //        DoLog("POPUP", "WM_SIZE", LOG_INFO);
        if (gAPI)
            gAPI->ResizeLogonWindow(hWnd);
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hWnd, &ps);
    }
    break;
    default:
        //sprintf_s(LogBuffer, LB_SIZE, "Windows msg 0x%X", message);
        //DoLog("POPUP", LogBuffer, LOG_INFO);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Message handler for preview window.
LRESULT CALLBACK PreviewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    SCROLLINFO si;

    // These variables are required by BitBlt. 
    static HDC hdcWin;           // window DC 
    static HDC hdcScreen;        // DC for entire screen 
    static HDC hdcScreenCompat;  // memory DC for screen 
    static HBITMAP hbmpCompat;   // bitmap handle to old DC 
    static BITMAP bmp;           // bitmap data structure 

    // These variables are required for horizontal scrolling. 
    static int xMinScroll;       // minimum horizontal scroll value 
    static int xCurrentScroll;   // current horizontal scroll value 
    static int xMaxScroll;       // maximum horizontal scroll value 

    // These variables are required for vertical scrolling. 
    static int yMinScroll;       // minimum vertical scroll value 
    static int yCurrentScroll;   // current vertical scroll value 
    static int yMaxScroll;       // maximum vertical scroll value 

    static Drawable * Doc;
    static DrawDetailsParams DocDrawDetails;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_CREATE:
        DoLog("PREVIEW", "WM_CREATE", LOG_DEBUG_VERBOSE);

        // Create a normal DC and a memory DC for the entire 
        // screen. The normal DC provides a snapshot of the 
        // screen contents. The memory DC keeps a copy of this 
        // snapshot in the associated bitmap. 
        hdcScreen = CreateDC(L"DISPLAY", (PCTSTR)NULL, (PCTSTR)NULL, (CONST DEVMODE*) NULL);
        hdcScreenCompat = CreateCompatibleDC(hdcScreen);

        // Retrieve the metrics for the bitmap associated with the 
        // regular device context. 
        bmp.bmBitsPixel = (BYTE)GetDeviceCaps(hdcScreen, BITSPIXEL);
        bmp.bmPlanes = (BYTE)GetDeviceCaps(hdcScreen, PLANES);
//        bmp.bmWidth = GetDeviceCaps(hdcScreen, HORZRES);
//        bmp.bmHeight = GetDeviceCaps(hdcScreen, VERTRES);
        bmp.bmWidth = 5000;
        bmp.bmHeight = 5000;
        DocDrawDetails.Rect.left = 0;
        DocDrawDetails.Rect.right = bmp.bmWidth;
        DocDrawDetails.Rect.top = 0;
        DocDrawDetails.Rect.bottom = bmp.bmHeight;

        // The width must be byte-aligned. 
        bmp.bmWidthBytes = ((bmp.bmWidth + 15) & ~15) / 8;

        // Create a bitmap for the compatible DC. 
        hbmpCompat = CreateBitmap(bmp.bmWidth, bmp.bmHeight, bmp.bmPlanes, bmp.bmBitsPixel, (CONST VOID*) NULL);

        // Select the bitmap for the compatible DC. 
        SelectObject(hdcScreenCompat, hbmpCompat);

        CreateWindow(_T("button"), _T("←"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            10, 10, 30, 30,
            hWnd, (HMENU)BTN_BUTTON_L, GetModuleHandle(NULL), NULL);
        CreateWindow(_T("button"), _T("→"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            40, 10, 30, 30,
            hWnd, (HMENU)BTN_BUTTON_R, GetModuleHandle(NULL), NULL);
        CreateWindow(_T("button"), _T("X"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            80, 10, 30, 30,
            hWnd, (HMENU)IDM_EXIT, GetModuleHandle(NULL), NULL);
        
//        PostMessage(hWnd, WM_PREPARE_POPUP, NULL, NULL);

        break;
    
    case WM_PREPARE_POPUP:
    {
        if (wParam) {
            Doc = (Drawable*)wParam;
        }

        if (Doc)
        {
            RECT rect = { 0, 0, 5000, 5000 };
            FillRect(hdcScreenCompat, &rect, (HBRUSH)(COLOR_GRAYTEXT + 1));

            DocDrawDetails.hDC = hdcScreenCompat;
            Doc->DrawPage(& DocDrawDetails, CurrentPage);
            bmp.bmWidth = DocDrawDetails.Rect.right - DocDrawDetails.Rect.left ;
            bmp.bmHeight = DocDrawDetails.Rect.bottom - DocDrawDetails.Rect.top;

            InvalidateRect(hWnd, NULL, TRUE);
        }

        // Initialize the horizontal scrolling variables. 
        xMinScroll = 0;
        xCurrentScroll = 0;
        xMaxScroll = 0;

        // Initialize the vertical scrolling variables. 
        yMinScroll = 0;
        yCurrentScroll = 0;
        yMaxScroll = 0;

        RECT rect;
        GetWindowRect(hWnd, &rect);
        if ((rect.right - rect.left) > bmp.bmWidth || (rect.bottom - rect.top) > bmp.bmHeight)
            SetWindowPos(hWnd, NULL, 0, 0, min(bmp.bmWidth, (rect.right - rect.left)), min(bmp.bmHeight, (rect.bottom - rect.top)), SWP_NOMOVE | SWP_NOZORDER);
        else
            PostMessage(hWnd, WM_SIZE, NULL, MAKELPARAM(rect.right, rect.bottom));
    }
    break;
    
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case BTN_BUTTON_L:
            if (CurrentPage > 0) {
                CurrentPage--;
                PostMessage(hWnd, WM_PREPARE_POPUP, NULL, NULL);
//                InvalidateRect(hWnd, NULL, TRUE);
//                ScrollOffset = { 0, 0 };
//                ImageSize = { 100, 100 };
            }
            break;
        case BTN_BUTTON_R:
            if (CurrentPage < NumPages - 1) {
                CurrentPage++;
                PostMessage(hWnd, WM_PREPARE_POPUP, NULL, NULL);
//                InvalidateRect(hWnd, NULL, TRUE);
//                ScrollOffset = { 0, 0 };
//                ImageSize = { 100, 100 };
            }
            break;
        }
    }
        break;
    case WM_DESTROY:
        SetForegroundWindow(GetParent(hWnd));
        hPreview = NULL;
        break;
    case WM_SIZE:
    {
        int xNewSize;
        int yNewSize;

        xNewSize = LOWORD(lParam);
        yNewSize = HIWORD(lParam);

        // The horizontal scrolling range is defined by 
        // (bitmap_width) - (client_width). The current horizontal 
        // scroll value remains within the horizontal scrolling range. 
        xMaxScroll = max(bmp.bmWidth - xNewSize, 0);
        xCurrentScroll = min(xCurrentScroll, xMaxScroll);
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = xMinScroll;
        si.nMax = bmp.bmWidth;
        si.nPage = xNewSize;
        si.nPos = xCurrentScroll;
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

        // The vertical scrolling range is defined by 
        // (bitmap_height) - (client_height). The current vertical 
        // scroll value remains within the vertical scrolling range. 
        yMaxScroll = max(bmp.bmHeight - yNewSize, 0);
        yCurrentScroll = min(yCurrentScroll, yMaxScroll);
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = yMinScroll;
        si.nMax = bmp.bmHeight;
        si.nPage = yNewSize;
        si.nPos = yCurrentScroll;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        InvalidateRect(hWnd, NULL, TRUE);

        break;
    }
    case WM_PAINT:
    {
        PRECT prect;

        hdc = BeginPaint(hWnd, &ps);
        // The coordinates of this rectangle are specified in the 
        // RECT structure to which prect points. 
        prect = &ps.rcPaint;

        BitBlt(ps.hdc,
            prect->left, prect->top,
            (prect->right - prect->left),
            (prect->bottom - prect->top),
            hdcScreenCompat,
            prect->left + xCurrentScroll,
            prect->top + yCurrentScroll,
            SRCCOPY);

        EndPaint(hWnd, &ps);

        break;
    }

    case WM_MOUSEWHEEL:
    {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        //if (zDelta < 0)
        //{ } else { }
        // Reset the current scroll position. 
        int yNewPos = yCurrentScroll - zDelta;
        // New position must be between 0 and the screen width. 
        yNewPos = max(0, yNewPos);
        yNewPos = min(yMaxScroll, yNewPos);

        // If the current position does not change, do not scroll.
        if (yNewPos == yCurrentScroll)
            break;

        yCurrentScroll = yNewPos;
        // Scroll the window. (The system repaints most of the 
        // client area when ScrollWindowEx is called; however, it is 
        // necessary to call UpdateWindow in order to repaint the 
        // rectangle of pixels that were invalidated.) 
        ScrollWindowEx(hWnd, 0, zDelta, (CONST RECT*) NULL,
            (CONST RECT*) NULL, (HRGN)NULL, (PRECT)NULL,
            SW_INVALIDATE);
        InvalidateRect(hWnd, NULL, TRUE);
        //        UpdateWindow(hWnd);

                // Reset the scroll bar. 
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = yCurrentScroll;
        //        InvalidateRect(hWnd, NULL, TRUE);
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
    }
        break;
    case WM_HSCROLL:
    {
        int xDelta;     // xDelta = new_pos - current_pos  
        int xNewPos;    // new position 
        int yDelta = 0;

        switch (LOWORD(wParam))
        {
            // User clicked the scroll bar shaft left of the scroll box. 
        case SB_PAGEUP:
            xNewPos = xCurrentScroll - 50;
            break;

            // User clicked the scroll bar shaft right of the scroll box. 
        case SB_PAGEDOWN:
            xNewPos = xCurrentScroll + 50;
            break;

            // User clicked the left arrow. 
        case SB_LINEUP:
            xNewPos = xCurrentScroll - 5;
            break;

            // User clicked the right arrow. 
        case SB_LINEDOWN:
            xNewPos = xCurrentScroll + 5;
            break;

            // User dragged the scroll box. 
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            xNewPos = HIWORD(wParam);
            break;

        default:
            xNewPos = xCurrentScroll;
        }

        // New position must be between 0 and the screen width. 
        xNewPos = max(0, xNewPos);
        xNewPos = min(xMaxScroll, xNewPos);

        // If the current position does not change, do not scroll.
        if (xNewPos == xCurrentScroll)
            break;

        // Determine the amount scrolled (in pixels). 
        xDelta = xNewPos - xCurrentScroll;

        // Reset the current scroll position. 
        xCurrentScroll = xNewPos;

        // Scroll the window. (The system repaints most of the 
        // client area when ScrollWindowEx is called; however, it is 
        // necessary to call UpdateWindow in order to repaint the 
        // rectangle of pixels that were invalidated.) 
        ScrollWindowEx(hWnd, -xDelta, -yDelta, (CONST RECT*) NULL,
            (CONST RECT*) NULL, (HRGN)NULL, (PRECT)NULL,
            SW_INVALIDATE);
        InvalidateRect(hWnd, NULL, TRUE);
//        UpdateWindow(hWnd);

        // Reset the scroll bar. 
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = xCurrentScroll;
//        InvalidateRect(hWnd, NULL, TRUE);
        SetScrollInfo(hWnd, SB_HORZ, &si, TRUE);

        break;
    }

    case WM_VSCROLL:
    {
        int xDelta = 0;
        int yDelta;     // yDelta = new_pos - current_pos 
        int yNewPos;    // new position 

        switch (LOWORD(wParam))
        {
            // User clicked the scroll bar shaft above the scroll box. 
        case SB_PAGEUP:
            yNewPos = yCurrentScroll - 50;
            break;

            // User clicked the scroll bar shaft below the scroll box. 
        case SB_PAGEDOWN:
            yNewPos = yCurrentScroll + 50;
            break;

            // User clicked the top arrow. 
        case SB_LINEUP:
            yNewPos = yCurrentScroll - 5;
            break;

            // User clicked the bottom arrow. 
        case SB_LINEDOWN:
            yNewPos = yCurrentScroll + 5;
            break;

            // User dragged the scroll box. 
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            yNewPos = HIWORD(wParam);
            break;

        default:
            yNewPos = yCurrentScroll;
        }

        // New position must be between 0 and the screen height. 
        yNewPos = max(0, yNewPos);
        yNewPos = min(yMaxScroll, yNewPos);

        // If the current position does not change, do not scroll.
        if (yNewPos == yCurrentScroll)
            break;

        // Determine the amount scrolled (in pixels). 
        yDelta = yNewPos - yCurrentScroll;

        // Reset the current scroll position. 
        yCurrentScroll = yNewPos;

        // Scroll the window. (The system repaints most of the 
        // client area when ScrollWindowEx is called; however, it is 
        // necessary to call UpdateWindow in order to repaint the 
        // rectangle of pixels that were invalidated.) 
        ScrollWindowEx(hWnd, -xDelta, -yDelta, (CONST RECT*) NULL,
            (CONST RECT*) NULL, (HRGN)NULL, (PRECT)NULL,
            SW_INVALIDATE);
//        UpdateWindow(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);

        // Reset the scroll bar. 
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = yCurrentScroll;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);

        break;
    }
    default:
        //sprintf_s(LogBuffer, LB_SIZE, "Windows msg 0x%X", message);
        //DoLog("POPUP", LogBuffer, LOG_INFO);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}


std::string RM_Name_FromList(HWND hWnd)
{
    std::unique_ptr<wchar_t> RMFile(new wchar_t[LB_SIZE]);
    int CurSel = ListBox_GetCurSel(hRMList);
    if (CurSel == -1) {
        DoLog("ONE_LOAD", "No document selected", LOG_WARNING);
        return "";
    }
    ListBox_GetText(hRMList, CurSel, RMFile.get());
    std::string sRMFile = ws2s(RMFile.get());
    return sRMFile;
}

ONE_ID OneID_FromList(HWND hWnd)
{
    ONE_ID OneID;
    int CurSel = ListBox_GetCurSel(hONEList);

    if (CurSel == -1) {
        DoLog("ONE_LOAD", "No document selected", LOG_WARNING);
        OneID.Empty = true;
        return OneID;
    }

    OneID.FileID = std::wstring((wchar_t*)ListBox_GetItemData(hONEList, CurSel));
    std::unique_ptr<wchar_t> Section(new wchar_t[LB_SIZE]);
    ListBox_GetText(hONEList, CurSel, (LPARAM)Section.get());
    std::string SectionName(ws2s(Section.get()));
    size_t sep = SectionName.find(" - ");
    OneID.Section = SectionName.substr(sep + 3);
    OneID.Notebook = SectionName.substr(0, sep);
    OneID.Empty = false;
    return OneID;
}

template <class PageType>
void RM_Load(RMDocFile<PageType> * ZF, std::string sRMFile)
{
    std::string Msg = "Starting RM Load : ";
    Msg.append(sRMFile);
    Msg.append("...");
    DoLog("MAIN", Msg.c_str(), LOG_INFO);

    std::unique_ptr<char> WorkingDir(new char[LB_SIZE]);
    GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir.get(), LB_SIZE, gszIniFileName);

    RMAPI::GetDoc(sRMFile);

    size_t Found = sRMFile.find_last_of("\\");
    if (Found != std::string::npos)
        sRMFile = sRMFile.substr(Found + 1);

    std::string Zipfile = WorkingDir.get();
    Zipfile.append(sRMFile);
    Zipfile.append(".rmdoc");
    NumPages = ZF->ExtractRMsFromZip(Zipfile.c_str());
    CurrentPage = 0;
}

template <class PageType>
bool ONE_Load(GraphDoc<PageType>* GD, ONE_ID OneID) {

    std::string Msg = "Starting ONE Load: ";
    Msg.append(OneID.Notebook);
    Msg.append(" - ");
    Msg.append(OneID.Section);
    Msg.append("...");
    DoLog("MAIN", Msg.c_str(), LOG_INFO);

    GD->Name = OneID.Section;

    if (OneID.FileID.empty())
        NumPages = GD->LoadDoc(OneID.Notebook, OneID.Section);
    else
        NumPages = GD->LoadPages(OneID.FileID.c_str());

    if (NumPages == -1) {
        // No Auth! Logon!
        DoLog("MAIN", L"No Auth!", LOG_INFO);
        return false;
    }
    CurrentPage = 0;
    return true;

}

void RM_Preview(HWND hWnd)
{
    if (ZF)
        delete ZF;
    ZF = new RMDocFile<WindowRMPage>();
    std::string RMFile = RM_Name_FromList(hWnd);
    if (RMFile.empty())
        return;

    RM_Load(ZF, RMFile);

    if (hPreview)
    {
        // Window already exists...
        ShowWindow(hPreview, SW_NORMAL);
        InvalidateRect(hPreview, NULL, TRUE);
    }
    else {
        hPreview = CreateWindowW(szPreviewWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
            CW_USEDEFAULT, 0, 500, 650, hWnd, nullptr, hInst, nullptr);
        if (!hPreview)
        {
            int err = GetLastError();
            std::wostringstream LB;
            LB << L"Creating Preview Error: 0x" << std::hex << std::uppercase << err;
            DoLog("MAIN", LB.str(), LOG_ERROR);
        }
        else
        {
            PostMessage(hPreview, WM_PREPARE_POPUP, (WPARAM)ZF, NULL);
            ShowWindow(hPreview, SW_NORMAL);
            UpdateWindow(hPreview);
        }
    }
}

void ONE_Preview(HWND hWnd)
{
    if (GD)
        delete GD;
    GD = new GraphDoc<WindowONEPage>(gAPI);
    ONE_ID OneID = OneID_FromList(hWnd);
    if (OneID.Empty)
        return;

    if (ONE_Load(GD, OneID))
    {
        if (hPreview)
        {
            // Window already exists...
            ShowWindow(hPreview, SW_NORMAL);
            InvalidateRect(hPreview, NULL, TRUE);
        }
        else {
            hPreview = CreateWindowW(szPreviewWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT, 0, 500, 650, hWnd, nullptr, hInst, nullptr);
            if (!hPreview)
            {
                int err = GetLastError();
                std::wostringstream LB;
                LB << L"Creating Preview Error: 0x" << std::hex << std::uppercase << err;
                DoLog("MAIN", LB.str(), LOG_ERROR);
            }
            else
            {
                PostMessage(hPreview, WM_PREPARE_POPUP, (WPARAM)GD, NULL);
                ShowWindow(hPreview, SW_NORMAL);
                UpdateWindow(hPreview);
            }
        }
    }
}

void RM_to_ONE(std::string RMFile, ONE_ID OneID, bool Notebook)
{
    // First, load the RM page
    if (! TOZF)
    {
        TOZF = new RMDocFile<ToOneRMPage>();
        RM_Load(TOZF, RMFile);
    }

    if (!gAPI)
        gAPI = new GraphAPI();

    if (GD)
        delete GD;
    GD = new GraphDoc<WindowONEPage>(gAPI);

    // Now do the conversion...
    for (int i = 0; i < NumPages; i++)
        TOZF->DrawPage((void*)GD, i);

    // And save it!
    std::wstring Msg = L"Starting ONE Save: ";
    Msg.append(s2ws(OneID.Notebook));
    Msg.append(L" - ");

    if (Notebook) {
        OneID.Section = TOZF->Name;
        Msg.append(s2ws(OneID.Section));
        Msg.append(L"...");
        DoLog("MAIN", Msg, LOG_INFO);
        GD->SaveDoc(OneID.Notebook, OneID.Section);
    }
    else {
        Msg.append(s2ws(OneID.Section));
        Msg.append(L"...");
        DoLog("MAIN", Msg, LOG_INFO);
        GD->SaveDoc(OneID.FileID.c_str());
    }
    CurrentPage = 0;
    
    DoLog("MAIN", L"... Done", LOG_INFO);
}

void ONE_to_RM(std::string RMFile, ONE_ID OneID, bool Notebook)
{
    // First, load the ONE page
    if (!gAPI)
        gAPI = new GraphAPI();

    if (!TOGD)
    {
        TOGD = new GraphDoc<ToRMOnePage>(gAPI);
        ONE_Load(TOGD, OneID);
    }

    if (ZF)
        delete ZF;
    ZF = new RMDocFile<WindowRMPage>();

    // Now convert...
    for (int i = 0; i < NumPages; i++)
        TOGD->DrawPage((void*)ZF, i);

    // And save the RM file#
    if (Notebook) {
        RMFile = TOGD->Name;
    }

    std::unique_ptr<char> WorkingDir(new char[LB_SIZE]);
    GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir.get(), LB_SIZE, gszIniFileName);
    
    std::wstring Msg = L"Starting RM Save: ";
    Msg.append(s2ws(RMFile));
    Msg.append(L"...");
    DoLog("MAIN", Msg, LOG_INFO);
    std::string TMPfile = WorkingDir.get();
    TMPfile.append("Output.rmdoc");
    if (ZF)
    {   
        ZF->Name = RMFile;
        NumPages = ZF->SaveRMsToZip(TMPfile.c_str());
    }

    RMAPI::SaveDoc(RMFile, WorkingDir.get());

    DoLog("MAIN",L"... Done", LOG_INFO);
}

void Timed(std::string RMFile, ONE_ID OneID) {
    std::string Key;

    if (TOZF)
        delete TOZF;
    TOZF = new RMDocFile<ToOneRMPage>();
    RM_Load(TOZF, RMFile);
    time_t ZFTime = TOZF->LastEditTime();
    std::unique_ptr<char> Buff(new char[LB_SIZE]);
    std::string Msg;
    struct tm datetime;
    localtime_s(&datetime, &ZFTime);
    strftime(Buff.get(), LB_SIZE, "%F %T", &datetime );
    Msg = "RM File edited at ";
    Msg.append(Buff.get());
    DoLog("MAIN", Msg.c_str(), LOG_INFO);

    if (!gAPI)
        gAPI = new GraphAPI();
    if (TOGD)
        delete TOGD;
    TOGD = new GraphDoc<ToRMOnePage>(gAPI);
    ONE_Load(TOGD, OneID);
    time_t GDTime = TOGD->LastEditTime();
    localtime_s(&datetime, &GDTime);
    strftime(Buff.get(), LB_SIZE, "%F %T", &datetime);
    Msg = "ONE File edited at ";
    Msg.append(Buff.get());
    DoLog("MAIN", Msg.c_str(), LOG_INFO);

    Key = TOZF->Name;
    Key.append(TOGD->Name);
    std::unique_ptr<char> LastUpdate(new char[LB_SIZE]);
    GetPrivateProfileStringA("TimedUpdate",Key.c_str(), "0", LastUpdate.get(), LB_SIZE, gszIniFileName);
    time_t LastUpdateTime = std::stoull(LastUpdate.get(), nullptr);
    if (GDTime < LastUpdateTime && ZFTime < LastUpdateTime)
    { 
        localtime_s(&datetime, &LastUpdateTime);
        strftime(Buff.get(), LB_SIZE, "%F %T", &datetime);
        Msg = "Neither file changed since last update at ";
        Msg.append(Buff.get());
        DoLog("MAIN", Msg.c_str(), LOG_INFO);
        return;
    }

    if (GDTime > ZFTime) {
        DoLog("MAIN", "OneNote is Later, performing ONE to RM...", LOG_INFO);
        ONE_to_RM(RMFile, OneID, true);
    }
    else {
        DoLog("MAIN", "RM is Later, performing RM to ONE...", LOG_INFO);
        RM_to_ONE(RMFile, OneID, true);
    }

    std::stringstream LU;
    LU << time(nullptr);
    WritePrivateProfileStringA("TimedUpdate", Key.c_str(), LU.str().c_str(), gszIniFileName);

    DoLog("MAIN", L"... Done", LOG_INFO);
}


int ProcessCommandLine(std::wstring CommandLine) {

	// First, create or attach to console so we can log our activity
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		// Probably command line - 
		FILE* fi = 0;
		freopen_s(&fi, "CONOUT$", "w", stdout);
		std::cout << std::endl;
	}
	else {
		if (AllocConsole())
		{
			FILE* fi = 0;
			freopen_s(&fi, "CONOUT$", "w", stdout);
			std::cout << std::endl;
		}
		else {
			// Error - can't do anything 
			DWORD iRet = GetLastError();
			LPTSTR errmessage;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, iRet, 0, (LPTSTR)&errmessage, 10, NULL);
			std::cout << ws2s(errmessage);
			MessageBox(NULL, errmessage, L"Can't create console", MB_OK);
			return 1;
		}
	}
	DoLog("CommandLine", L"Command Line mode selected", LOG_DEBUG);

	// Parse out command line switches
	std::wstring Input = ParseCommandLine(L"-I", CommandLine);
	std::wstring Output = ParseCommandLine(L"-O", CommandLine);
    std::wstring Mode = ParseCommandLine(L"-M", CommandLine);
    std::wstring Loop = ParseCommandLine(L"-L", CommandLine);

	if (Input.empty() || (Output.empty() && Mode == L"R") || Mode.empty() || (Mode != L"R" && Mode != L"O" && Mode != L"T"))
	{
		DoLog("CommandLine", L"Incorrect Parameters", LOG_ERROR);
		std::cout << "USAGE:" << std::endl << std::endl;
		std::cout << "OneNoteToRM.exe -M <Mode> -I <Input Document> -O <Output Document>" << std::endl << std::endl;
		std::cout << "-M R: Remarkable to OneNote" << std::endl;
        std::cout << "      -I is RM document name" << std::endl;
        std::cout << "      -O is mandatory as name of Notebook to insert section into" << std::endl;
        std::cout << "-M O: OneNote to Remarkable" << std::endl;
        std::cout << "      -I use format Notebook/Section" << std::endl;
        std::cout << "      -O is ignored" << std::endl;
        std::cout << "-M T: Time mode - either direction based on most recently updated" << std::endl;
        std::cout << "      (Will do nothing if neither is updated since last T mode invoked)" << std::endl;
        std::cout << "      -I - OneNote name: use format Notebook/Section" << std::endl;
        std::cout << "      -O - RM document Name" << std::endl;
        std::cout << "-L: Loop mode; will perform the action, wait for specified number(in seconds) and repeat indefinitely" << std::endl;
            
        return 1;
	}

    do
    {
        // And Do it!
        switch (Mode[0])
        {
        case L'R':
        {
            // RM to One mode
            ONE_ID OneID;
            OneID.Notebook = ws2s(Output);
            RM_to_ONE(ws2s(Input), OneID, true);
        }
        break;
        case 'O':
        {
            // One to RM Mode
            ONE_ID OneID;
            std::string SectionName(ws2s(Input));
            size_t sep = SectionName.find("/");
            OneID.Notebook = SectionName.substr(0, sep);
            OneID.Section = SectionName.substr(sep + 1);

            ONE_to_RM(ws2s(Output), OneID, true);
        }
        break;
        case 'T':
        {
            // Query mode
            ONE_ID OneID;
            std::string SectionName(ws2s(Input));
            size_t sep = SectionName.find("/");
            OneID.Notebook = SectionName.substr(0, sep);
            OneID.Section = SectionName.substr(sep + 1);

            Timed(ws2s(Output), OneID);
        }
        break;
        }

        DoLog("CommandLine", L"... Done", LOG_INFO);

        if (!Loop.empty())
        {
            std::chrono::seconds SS(std::stoi(Loop));
            std::wostringstream Msg;
            Msg << "Waiting for " << SS ;
            DoLog("Command Line", Msg.str(), LOG_INFO);
            std::this_thread::sleep_for(SS);
        }

    } while (!Loop.empty());

	return 0;
}

inline void ltrim(std::wstring& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
        return !std::isspace(ch);
        }));
}
inline void rtrim(std::wstring& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

std::wstring ParseCommandLine(std::wstring Flag, std::wstring CommandLine)
{
    std::wstring result;
    size_t found = CommandLine.find(Flag);
    if (found == std::string::npos)
        return result;

    found += Flag.length();
    size_t end = CommandLine.find(L"-", found);
    result = CommandLine.substr(found, (end - found));
    rtrim(result);
    ltrim(result);

    return result;
}

