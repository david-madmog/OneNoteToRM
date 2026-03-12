// DOCXToRM.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "OneNoteToRM.h"
#include "RMDocFile.h"
#include "WindowRMPage.h"
#include "ToOneRMPage.h"
#include "WindowONEPage.h"
#include "RMTestFileBuilder.h"
#include "GraphDoc.h"
#include "RMAPI.h"
#include <gdiplus.h>
#include <windowsx.h>

using namespace Gdiplus;

constexpr auto MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR szPopupWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR szPreviewWindowClass[MAX_LOADSTRING];            // the main window class name

#define MODE_RM 1
#define MODE_ONE 2

RMDocFile<WindowRMPage>* ZF;
RMDocFile<ToOneRMPage>* TOZF;
GraphDoc<WindowONEPage>* GD;
GraphAPI* gAPI;
int NumPages;
int CurrentPage;
//WNDPROC oldSDProc;
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
//LRESULT CALLBACK    ODStaticWndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam);


// Window controls... create and lay out
HWND hRMList;
HWND hONEList;
HWND hListBox;
HWND hImage;

HWND RMPreview;
HWND ONEPreview;
HWND hRMToOne;
HWND hONEToRM;

HWND RM1;
HWND RM2;
HWND RM3;
HWND ON1;



char gszIniFileName[] = ".\\OneNoteToRM.ini";

char * LogBuffer = new char[LB_SIZE];

const char* LogLevelName[] = {
    "VERB",
    "DBUG",
    "INFO",
    "WARN",
    "*ERR"
};

const LogLevel CurrentLevel = LogLevel::LOG_DEBUG;

void DoLog(const char* Class, const wchar_t* Msg, LogLevel Level)
{
    std::wostringstream buff;
    std::time_t time = std::time({});
    char timeString[std::size("yyyy-mm-ddThh:mm:ssZ")];
    struct tm tmDest;
    gmtime_s(&tmDest, &time);
    std::strftime(std::data(timeString), std::size(timeString), "%F %T", &tmDest);

    buff << L"[" << timeString << L"][" << LogLevelName[Level] << L"][" << Class << L"]:" << Msg;
    if (Level >= CurrentLevel)
    {
        LRESULT NewItem = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buff.str().c_str());
        SendMessage(hListBox, LB_SETTOPINDEX, NewItem, NULL);
    }

    buff << std::endl;
    OutputDebugString(buff.str().c_str());
}

void DoLog(const char * Class, const char* Msg, LogLevel Level)
{
    std::wostringstream buff;
    std::time_t time = std::time({});
    char timeString[std::size("yyyy-mm-ddThh:mm:ssZ")];
    struct tm tmDest;
    gmtime_s(&tmDest, &time);
    std::strftime(std::data(timeString), std::size(timeString), "%F %T", &tmDest);

    buff << L"[" << timeString << L"][" << LogLevelName[Level] << L"][" << Class << L"]:" << Msg;
    if (Level >= CurrentLevel)
    {
        LRESULT NewItem = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buff.str().c_str());
        SendMessage(hListBox, LB_SETTOPINDEX, NewItem, NULL);
    }

    buff << std::endl;
    OutputDebugString(buff.str().c_str());
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
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
    if (!InitInstance (hInstance, nCmdShow))
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

    return (int) msg.wParam;
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
    SetWindowPos(hRMToOne, NULL, (9 * ColW)/2 + hPad - 15, RowH + hPad, 30, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(hONEToRM, NULL, (9 * ColW)/2 + hPad - 15, RowH + 2 * hPad + 30, 30, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(RMPreview, NULL, hPad, (3 * RowH + hPad), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(ONEPreview, NULL, (5 * ColW + hPad), (3 * RowH + hPad), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    SetWindowPos(RM1, NULL, hPad, (3 * RowH + 2 * hPad + 30), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(RM2, NULL, 2 * hPad + 120, (3 * RowH + 2 * hPad + 30), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    SetWindowPos(RM3, NULL, 3 * hPad + 240, (3 * RowH + 2 * hPad + 30), 150, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    SetWindowPos(ON1, NULL, (5 * ColW + hPad), (3 * RowH + 2 * hPad + 30), 120, 30, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    SetWindowPos(hListBox, NULL, hPad, (4 * RowH + hPad), (9 * ColW - hPad), (RowH - hPad), SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
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
        hWnd, (HMENU)LST_LISTBOX,
        hInst, 0);

    //CreateWindow(_T("button"), _T("Login to Microsoft"),
    //    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
    //    10, 80, 120, 30,
    //    hWnd, (HMENU)BTN_BUTTON_LOGIN, GetModuleHandle(NULL), NULL);
    RMPreview = CreateWindow(_T("button"), _T("Preview"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        10, 10, 120, 30,
        hWnd, (HMENU)BTN_RM_PREVIEW_BUTTON, GetModuleHandle(NULL), NULL);
    RM1 = CreateWindow(_T("button"), _T("Save RMDOC"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        130, 10, 120, 30,
        hWnd, (HMENU)BTN_BUTTONSAVE, GetModuleHandle(NULL), NULL);
    RM2 = CreateWindow(_T("button"), _T("Test"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        250, 10, 120, 30,
        hWnd, (HMENU)BTN_BUTTON_TEST, GetModuleHandle(NULL), NULL);
    RM3 = CreateWindow(_T("button"), _T("Download/Upload"),
        WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
        370, 10, 150, 30,
        hWnd, (HMENU)CHK_RELOAD, GetModuleHandle(NULL), NULL);


    hRMToOne= CreateWindow(_T("button"), _T("→"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        55, 45, 30, 30,
        hWnd, (HMENU)BTN_RM_TO_ONE, GetModuleHandle(NULL), NULL);

    hONEToRM = CreateWindow(_T("button"), _T("←"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        175, 45, 30, 30,
        hWnd, (HMENU)BTN_ONE_TO_RM, GetModuleHandle(NULL), NULL);



    ONEPreview = CreateWindow(_T("button"), _T("Preview"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        10, 80, 120, 30,
        hWnd, (HMENU)BTN_ONE_PREVIEW_BUTTON, GetModuleHandle(NULL), NULL);

    ON1 = CreateWindow(_T("button"), _T("Save ONE doc"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        130, 80, 120, 30,
        hWnd, (HMENU)BTN_BUTTON_ONESAVE, GetModuleHandle(NULL), NULL);


    hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, _T("listbox"),
        _T("caption.c_str()"),
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        10, 120, 500, 500,
        hWnd, (HMENU)LST_LISTBOX,
        hInst, 0);
    SendMessage(hListBox, LB_SETHORIZONTALEXTENT, 1000, 0);

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
            {
                wchar_t * RMFile = new wchar_t[LB_SIZE];
                int CurSel = ListBox_GetCurSel(hRMList);
                ListBox_GetText(hRMList, CurSel, RMFile);
                char* sRMFile = new char[LB_SIZE];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, sRMFile, LB_SIZE, RMFile, _TRUNCATE);

                char* WorkingDir = new char[LB_SIZE];
                GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir, LB_SIZE, gszIniFileName);

                if (IsDlgButtonChecked(hWnd, CHK_RELOAD))
                {
                    RMAPI::GetDoc(sRMFile);
                }

                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting  RM LOAD..."));
                if (ZF) 
                    delete ZF;
                ZF = new RMDocFile<WindowRMPage>();

                std::string Zipfile = WorkingDir;
                Zipfile.append(sRMFile);
                Zipfile.append(".rmdoc");
                NumPages = ZF->ExtractRMsFromZip(Zipfile.c_str());
                CurrentPage = 0;

                if (hPreview)
                { 
                    // Window already exists...
                    ShowWindow(hPreview, SW_NORMAL);
                    InvalidateRect(hPreview, NULL, TRUE);
                } else {
                    hPreview = CreateWindowW(szPreviewWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                        CW_USEDEFAULT, 0, 500, 650, hWnd, nullptr, hInst, nullptr);
                    if (!hPreview)
                    {
                        int err = GetLastError();
                        sprintf_s(LogBuffer, LB_SIZE, "Creating perview error 0x%X", err);
                        DoLog("MAIN", LogBuffer, LOG_ERROR);
                    }
                    else
                    {
                        PostMessage(hPreview, WM_PREPARE_POPUP, (WPARAM)ZF, NULL);
                        ShowWindow(hPreview, SW_NORMAL);
                        UpdateWindow(hPreview);
                    }
                }

                delete[] WorkingDir;
            }
                break;
            case BTN_BUTTONSAVE:
            {
                char* WorkingDir = (char*)malloc(LB_SIZE);
                GetPrivateProfileStringA("RMFILE", "SaveDoc", "", WorkingDir, LB_SIZE, gszIniFileName);

                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting  RM SAVE..."));
                if (ZF)
                    NumPages = ZF->SaveRMsToZip(WorkingDir);

                //Result = exec("copy Output.rmdoc \"Conversion test.rmdoc\" /B /Y");
                //DoLog("MAIN", Result.c_str(), LOG_INFO);

                if (IsDlgButtonChecked(hWnd, CHK_RELOAD))
                {
                //    Result = exec("rmapi put \"Conversion test.rmdoc\" --force");
                //    DoLog("MAIN", Result.c_str(), LOG_INFO);
                }
                free(WorkingDir);
            }
                break;
            case CHK_RELOAD:
                if (IsDlgButtonChecked(hWnd, CHK_RELOAD)) {
                    CheckDlgButton(hWnd, CHK_RELOAD, BST_UNCHECKED);
                }
                else {
                    CheckDlgButton(hWnd, CHK_RELOAD, BST_CHECKED);
                }
                break;
            case BTN_BUTTON_TEST:
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting TEST..."));
                ZF = new RMTestFileBuilder<WindowRMPage>;
                NumPages = ((RMTestFileBuilder<WindowRMPage>*)ZF)->Build();
                CurrentPage = 0;
                //InvalidateRect(hImage, NULL, TRUE);
                break;
            case BTN_ONE_PREVIEW_BUTTON:
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting ONENOTE LOAD..."));

                int CurSel = ListBox_GetCurSel(hONEList);
                wchar_t* ONEFileID = (wchar_t*)SendMessage(hONEList, LB_GETITEMDATA, CurSel, NULL);
                if (!gAPI)
                    gAPI = new GraphAPI();

                if (GD)
                    delete GD;
                GD = new GraphDoc<WindowONEPage>(gAPI);
                    
                NumPages = GD->LoadPages(ONEFileID);
                if (NumPages == -1) {
                    // No Auth! Logon!
                    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("No Auth, starting popup"));
                    hLoginPopup = CreateWindowW(szPopupWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
                        CW_USEDEFAULT, 0, 500, 650, hWnd, nullptr, hInst, nullptr);

                    if (!hLoginPopup) {
                        DWORD err = GetLastError();
                        sprintf_s(LogBuffer, LB_SIZE, "Creating popup error 0x%X", err);
                        DoLog("POPUP", LogBuffer, LOG_ERROR);
                    }
                    else {
                        ShowWindow(hLoginPopup, SW_NORMAL);
                        UpdateWindow(hLoginPopup);
                        PostMessage(hLoginPopup, WM_LOGINTOMS, NULL, (LPARAM) gAPI);
                    }

                }
                CurrentPage = 0;

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
                        sprintf_s(LogBuffer, LB_SIZE, "Creating perview error 0x%X", err);
                        DoLog("MAIN", LogBuffer, LOG_ERROR);
                    }
                    else
                    {
                        PostMessage(hPreview, WM_PREPARE_POPUP, (WPARAM)GD, NULL);
                        ShowWindow(hPreview, SW_NORMAL);
                        UpdateWindow(hPreview);
                    }
                }
            }
			break;
            case BTN_BUTTON_ONESAVE:
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting ONENOTE SAVE..."));

                char* Setting = new char[LB_SIZE];
                GetPrivateProfileStringA("OneNote", "Notebook", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Notebook{ Setting };
                GetPrivateProfileStringA("OneNote", "Section", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Section{ Setting };
                if (GD)
                    GD->SaveDoc(Notebook, Section);
            }
            break;
            case BTN_RM_TO_ONE:
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting RM TO ONE..."));

                // First, load the RM page
                wchar_t* RMFile = new wchar_t[LB_SIZE];
                int CurSel = ListBox_GetCurSel(hRMList);
                ListBox_GetText(hRMList, CurSel, RMFile);
                char* sRMFile = new char[LB_SIZE];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, sRMFile, LB_SIZE, RMFile, _TRUNCATE);

                char* WorkingDir = new char[LB_SIZE];
                GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir, LB_SIZE, gszIniFileName);

                if (IsDlgButtonChecked(hWnd, CHK_RELOAD))
                {
                    RMAPI::GetDoc(sRMFile);
                }

                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting RM LOAD..."));
                TOZF = new RMDocFile<ToOneRMPage>();
                std::string Zipfile = WorkingDir;
                Zipfile.append(sRMFile);
                Zipfile.append(".rmdoc");
                NumPages = TOZF->ExtractRMsFromZip(Zipfile.c_str());

                if (!gAPI)
                    gAPI = new GraphAPI();

                GD = new GraphDoc<WindowONEPage>(gAPI);

                // Now do the conversion...
                for (int i = 0; i < NumPages; i++)
                    TOZF->DrawPage((void*)GD, i);

                // And save it!
                CurSel = ListBox_GetCurSel(hONEList);
                if (CurSel != -1)
                {
                    wchar_t* ONEFileID = (wchar_t*)SendMessage(hONEList, LB_GETITEMDATA, CurSel, NULL);
                    GD->SaveDoc(ONEFileID);
                    CurrentPage = 0;
                }
            }
            break;
            case BTN_ONE_TO_RM:
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting ONE TO RM..."));

                char* Setting = new char[LB_SIZE];
                GetPrivateProfileStringA("OneNote", "Notebook", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Notebook{ Setting };
                GetPrivateProfileStringA("OneNote", "Section", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Section{ Setting };
                if (GD)
                    GD->SaveDoc(Notebook, Section);
            }
            break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_LOADRMDOCS:
    {
        std::vector<std::wstring> Docs;
        RMAPI::ListDocs(Docs);
        for (auto& Doc : Docs) {
            ListBox_AddString(hRMList, Doc.c_str());
        }
    }
        break;
    case WM_LOADONEDOCS:
    {
        std::vector<ONE_Section> Sections;
        if (!gAPI)
            gAPI = new GraphAPI();
        gAPI->EnsureConnected();

        gAPI->ListSections(Sections);
        for (auto& Section : Sections) {
            std::wstring Name = Section.Notebook;
            Name.append(L" - ");
            Name.append(Section.Section);
            LRESULT Index = ListBox_AddString(hONEList, Name.c_str());
            wchar_t* SID = new wchar_t[1023];
            wcscpy_s(SID, 1023, Section.ID.c_str());
            SendMessage(hONEList, LB_SETITEMDATA, Index, (LPARAM)SID);
        }
    }
        break;
    //case WM_PAINT:
    //    {
    //        PAINTSTRUCT ps;
    //        HDC hdc = BeginPaint(hWnd, &ps);
    //        // TODO: Add any drawing code that uses hdc here...
    //        EndPaint(hWnd, &ps);
    //    }
    //    break;
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
        DoLog("POPUP", "WM_CREATE", LOG_INFO);
        break;
    case WM_NCCREATE:
        DoLog("POPUP", "WM_NCCREATE", LOG_INFO);
        break;
    case WM_LOGINTOMS:
        DoLog("POPUP", "WM_LOGINTOMS", LOG_INFO);
        if (!gAPI)
            gAPI = new GraphAPI();
        gAPI->LoginToMicrosoft(hWnd);
        SetTimer(hWnd, WM_LOGINTOMS, 3000, (TIMERPROC)NULL);
        break;
    case WM_TIMER:
        DoLog("POPUP", "WM_TIMER", LOG_INFO);
        KillTimer(hWnd, WM_LOGINTOMS);
        if (gAPI)
            gAPI->ResizeLogonWindow(hWnd);
        break;
    case WM_DONELOGINTOMS:
        DoLog("POPUP", "WM_DONELOGINTOMS", LOG_INFO);
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
        DoLog("PREVIEW", "WM_CREATE", LOG_INFO);

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

