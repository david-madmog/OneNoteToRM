// DOCXToRM.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "DOCXToRM.h"
#include "RMZipFile.h"

constexpr auto MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

HWND hListBox;
HWND hImage;
RMZipFile* ZF;
int NumPages;
int CurrentPage;
WNDPROC oldSDProc;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    ODStaticWndProc(HWND hwnd, UINT Message, WPARAM wparam, LPARAM lparam);



void DoLog(const char* Msg, LogLevel Level)
{
    size_t newsize = strlen(Msg) + 1;
    wchar_t* wcstring = new wchar_t[newsize];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, wcstring, newsize, Msg, _TRUNCATE);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)wcstring);
    delete[] wcstring;
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DOCXTORM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

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

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DOCXTORM));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_DOCXTORM);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
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
//    const char WorkingDir[] = "C:\\Users\\david\\OneDrive\\Documents\\Development\\ReMarkable\\DOCXToRM\\To Do list.zip";
//
//   const char WorkingDir[] = "C:\\Users\\david\\OneDrive\\Documents\\Development\\ReMarkable\\DOCXToRM\\To Do list.rmdoc";
//    const char WorkingDir[] = "C:\\Users\\david\\OneDrive\\Documents\\Development\\ReMarkable\\DOCXToRM\\ICE.rmdoc";
    const char WorkingDir[] = "C:\\Users\\david\\OneDrive\\Documents\\Development\\ReMarkable\\DOCXToRM\\Jobs.rmdoc";

    switch (message)
    {
    case WM_CREATE:
        CreateWindow(_T("button"), _T("Go!"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            10, 10, 120, 30,
            hWnd, (HMENU)BTN_BUTTON, GetModuleHandle(NULL), NULL);
        CreateWindow(_T("button"), _T("<"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            130, 10, 30, 30,
            hWnd, (HMENU)BTN_BUTTON_L, GetModuleHandle(NULL), NULL);
        CreateWindow(_T("button"), _T(">"),
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            160, 10, 30, 30,
            hWnd, (HMENU)BTN_BUTTON_R, GetModuleHandle(NULL), NULL);


        hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, _T("listbox"),
            _T("caption.c_str()"),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_DISABLENOSCROLL | LBS_NOTIFY,
            10, 40, 500, 500,
            hWnd, (HMENU)LST_LISTBOX,
            hInst, 0); 

        hImage = CreateWindow(_T("static"), _T("DrawBox"), WS_CHILD | WS_VISIBLE | SS_OWNERDRAW ,
            520, 10, 500, 500,
            hWnd, (HMENU)MYDRAW, NULL, NULL);
        oldSDProc = (WNDPROC)SetWindowLongPtr(hImage, GWLP_WNDPROC, (LPARAM)ODStaticWndProc);

    break;    
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case BTN_BUTTON:
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM) _T("Starting..."));
                ZF = new RMZipFile();

                NumPages = ZF->ExtractRMsFromZip(WorkingDir);
                CurrentPage = 0;
                InvalidateRect(hImage, NULL, TRUE);
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
		}

		EndPaint(hwnd, &ps);
    }
    return 0;
}
