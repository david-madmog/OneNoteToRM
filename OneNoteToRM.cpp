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
#include <gdiplus.h>

using namespace Gdiplus;

constexpr auto MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
WCHAR szPopupWindowClass[MAX_LOADSTRING];            // the main window class name

HWND hListBox;
HWND hImage;
RMDocFile<WindowRMPage>* ZF;
RMDocFile<ToOneRMPage>* TOZF;
GraphDoc<WindowONEPage>* GD;
int NumPages;
int CurrentPage;
WNDPROC oldSDProc;
HWND hLoginPopup;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
ATOM                MyRegisterPopupClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    PopupWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ODStaticWndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam);

//char gszIniFileName[] = "C:\\Users\\david\\OneDrive\\Documents\\Development\\ReMarkable\\OneNoteToRM\\OneNoteToRM.ini";
char gszIniFileName[] = ".\\OneNoteToRM.ini";

char * LogBuffer = new char[LB_SIZE];
char LocalLogBuff[LB_SIZE];
wchar_t LocalWLogBuff[LB_SIZE];

const char* LogLevelName[] = {
    "VERB" ,
    "DBUG",
    "INFO",
    "WARN",
    "*ERR"
};

const LogLevel CurrentLevel = LogLevel::LOG_DEBUG;

void DoLog(const char * Class, const char* Msg, LogLevel Level)
{
    std::wostringstream buff;

    buff << L"[" << LogLevelName[Level] << L"][" << Class << L"]:" << Msg;
    if (Level >= CurrentLevel)
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)buff.str().c_str());

    buff << std::endl;
    OutputDebugString(buff.str().c_str());
}

static std::string exec(const char* cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
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
    MyRegisterClass(hInstance);
    MyRegisterPopupClass(hInstance);

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
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DOCXTORM);
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
      CW_USEDEFAULT, 0, 1500, 800, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


void LoadControls(HWND hWnd) {
    CreateWindow(_T("button"), _T("←"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        460, 45, 30, 30,
        hWnd, (HMENU)BTN_BUTTON_L, GetModuleHandle(NULL), NULL);
    CreateWindow(_T("button"), _T("→"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        490, 45, 30, 30,
        hWnd, (HMENU)BTN_BUTTON_R, GetModuleHandle(NULL), NULL);
    //CreateWindow(_T("button"), _T("Login to Microsoft"),
    //    WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
    //    10, 80, 120, 30,
    //    hWnd, (HMENU)BTN_BUTTON_LOGIN, GetModuleHandle(NULL), NULL);
    CreateWindow(_T("button"), _T("Load RMDOC"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        10, 10, 120, 30,
        hWnd, (HMENU)BTN_BUTTON, GetModuleHandle(NULL), NULL);
    CreateWindow(_T("button"), _T("Save RMDOC"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        130, 10, 120, 30,
        hWnd, (HMENU)BTN_BUTTONSAVE, GetModuleHandle(NULL), NULL);
    CreateWindow(_T("button"), _T("Test"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        250, 10, 120, 30,
        hWnd, (HMENU)BTN_BUTTON_TEST, GetModuleHandle(NULL), NULL);
    CreateWindow(_T("button"), _T("Download/Upload"),
        WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
        370, 10, 150, 30,
        hWnd, (HMENU)CHK_RELOAD, GetModuleHandle(NULL), NULL);


    CreateWindow(_T("button"), _T("↓"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        55, 45, 30, 30,
        hWnd, (HMENU)BTN_RM_TO_ONE, GetModuleHandle(NULL), NULL);

    CreateWindow(_T("button"), _T("↑"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        175, 45, 30, 30,
        hWnd, (HMENU)BTN_ONE_TO_RM, GetModuleHandle(NULL), NULL);



    CreateWindow(_T("button"), _T("Load ONE doc"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        10, 80, 120, 30,
        hWnd, (HMENU)BTN_BUTTON_ONE, GetModuleHandle(NULL), NULL);

    CreateWindow(_T("button"), _T("Save ONE doc"),
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

    hImage = CreateWindow(_T("static"), _T("DrawBox"), WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | WS_VSCROLL | WS_HSCROLL,
        520, 10, 5000, 5000,
        hWnd, (HMENU)MYDRAW, NULL, NULL);
    oldSDProc = (WNDPROC)SetWindowLongPtr(hImage, GWLP_WNDPROC, (LPARAM)ODStaticWndProc);

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
    std::string Result= "";

    switch (message)
    {
    case WM_CREATE:
        LoadControls(hWnd);

    break;    
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case BTN_BUTTON:
            {
                char* WorkingDir = (char *) malloc(LB_SIZE);
                GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir, LB_SIZE, gszIniFileName);

                if (IsDlgButtonChecked(hWnd, CHK_RELOAD))
                {
                    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Pre-load RM LOAD..."));
                    Result = exec("rmapi get \"Conversion test\"");
                    DoLog("MAIN", Result.c_str(), LOG_INFO);
                }

                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting  RM LOAD..."));
                ZF = new RMDocFile<WindowRMPage>();

                NumPages = ZF->ExtractRMsFromZip(WorkingDir);
                CurrentPage = 0;
                InvalidateRect(hImage, NULL, TRUE);
                free(WorkingDir);
            }
                break;
            case BTN_BUTTONSAVE:
            {
                char* WorkingDir = (char*)malloc(LB_SIZE);
                GetPrivateProfileStringA("RMFILE", "SaveDoc", "", WorkingDir, LB_SIZE, gszIniFileName);

                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting  RM SAVE..."));
                if (ZF)
                    NumPages = ZF->SaveRMsToZip(WorkingDir);

                Result = exec("copy Output.rmdoc \"Conversion test.rmdoc\" /B /Y");
                DoLog("MAIN", Result.c_str(), LOG_INFO);

                if (IsDlgButtonChecked(hWnd, CHK_RELOAD))
                {
                    Result = exec("rmapi put \"Conversion test.rmdoc\" --force");
                    DoLog("MAIN", Result.c_str(), LOG_INFO);
                }
                free(WorkingDir);
            }
                break;
            case BTN_BUTTON_L:
                if (CurrentPage > 0) {
                    CurrentPage--;
                    InvalidateRect(hImage, NULL, TRUE);
                }
                break;
            case BTN_BUTTON_R:
                if (CurrentPage < NumPages - 1) {
                    CurrentPage++;
                    InvalidateRect(hImage, NULL, TRUE);
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
                InvalidateRect(hImage, NULL, TRUE);
                break;
            case BTN_BUTTON_ONE:
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting ONENOTE LOAD..."));

                char Setting[LB_SIZE] ;
                GetPrivateProfileStringA("OneNote", "Notebook", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Notebook{ Setting };
                GetPrivateProfileStringA("OneNote", "Section", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Section{ Setting };
                if (!GD)
                    GD = new GraphDoc<WindowONEPage>();
                    
                NumPages = GD->LoadDoc(Notebook, Section);
                if (NumPages == -1) {
                    // No Auth! Logon!
                    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("No Auth, starting popup"));
                    hLoginPopup = CreateWindowW(szPopupWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT, 0, 500, 650, hWnd, nullptr, hInst, nullptr);

                    if (!hLoginPopup) {
                        DWORD err = GetLastError();
                        sprintf_s(LogBuffer, LB_SIZE, "Creating popup error 0x%X", err);
                        DoLog("POPUP", LogBuffer, LOG_ERROR);
                    }
                    else {
                        ShowWindow(hLoginPopup, SW_NORMAL);
                        UpdateWindow(hLoginPopup);
                        PostMessage(hLoginPopup, WM_LOGINTOMS, NULL, NULL);
                    }

                }
                CurrentPage = 0;
                InvalidateRect(hImage, NULL, TRUE);
            }
			break;
            case BTN_BUTTON_ONESAVE:
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting ONENOTE SAVE..."));

                char* Setting = (char*)malloc(LB_SIZE);
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
                char* WorkingDir = (char*)malloc(LB_SIZE);
                GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir, LB_SIZE, gszIniFileName);

                if (IsDlgButtonChecked(hWnd, CHK_RELOAD))
                {
                    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Pre-load RM LOAD..."));
                    Result = exec("rmapi get \"Conversion test\"");
                    DoLog("MAIN", Result.c_str(), LOG_INFO);
                }

                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting RM LOAD..."));
                TOZF = new RMDocFile<ToOneRMPage>();
                NumPages = TOZF->ExtractRMsFromZip(WorkingDir);

                // Second, create an empty OneNote doc to transfer into
                char* Setting = (char*)malloc(LB_SIZE);
                GetPrivateProfileStringA("OneNote", "Notebook", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Notebook{ Setting };
                GetPrivateProfileStringA("OneNote", "Section", "", Setting, LB_SIZE, gszIniFileName);
                const std::string Section{ Setting };
                GD = new GraphDoc<WindowONEPage>();

                // Now do the conversion...
                for (int i = 0; i < NumPages; i++)
                    TOZF->DrawPage((void*)GD, i);

                // And save it!
                GD->SaveDoc(Notebook, Section);
                CurrentPage = 0;
                InvalidateRect(hImage, NULL, TRUE);
            }
            break;
            case BTN_ONE_TO_RM:
            {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)_T("Starting ONE TO RM..."));

                char* Setting = (char*)malloc(LB_SIZE);
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
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
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
        if (!GD)
            GD = new GraphDoc<WindowONEPage>();
        GD->LoginToMicrosoft(hWnd);
        SetTimer(hWnd, WM_LOGINTOMS, 3000, (TIMERPROC)NULL);
        break;
    case WM_TIMER:
        DoLog("POPUP", "WM_TIMER", LOG_INFO);
        KillTimer(hWnd, WM_LOGINTOMS);
        if (GD)
            GD->Resize(hWnd);
        break;
    case WM_DONELOGINTOMS:
        DoLog("POPUP", "WM_DONELOGINTOMS", LOG_INFO);
        GD->SetLoginCode((wchar_t*)lParam);
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        SetForegroundWindow(GetParent(hWnd));
        break;
    case WM_SIZE:
//        DoLog("POPUP", "WM_SIZE", LOG_INFO);
        if (GD)
            GD->Resize(hWnd);
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


// Message Handler for own draw static image box
LRESULT CALLBACK ODStaticWndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam) {

    if (Message == WM_PAINT) {
        PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);

		RECT rt = { 0 };
		GetClientRect(hwnd, &rt);
		HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		HGDIOBJ holdPen = SelectObject(hDC, pen);
		MoveToEx(hDC, rt.left, rt.top, NULL);
		LineTo(hDC, rt.right, rt.top);
		LineTo(hDC, rt.right, rt.bottom);
		LineTo(hDC, rt.left, rt.bottom);
		LineTo(hDC, rt.left, rt.top);
		SelectObject(hDC, holdPen);
		DeleteObject(pen);
		pen = NULL;

		if (ZF) {
			ZF->DrawPage(hDC, CurrentPage);
		} else if (GD) {
			GD->DrawPage(hDC, CurrentPage);
        }

		EndPaint(hwnd, &ps);
    }
    return 0;
}
