#pragma once

/*******************************************************************************

	OneNoteToRMDLL.h

	Header for functions to call DLL - can be used for C++ apps calling the DLL, 
		or for template to generate imports for other languages. Also used for the 
		DLL itself.

	(C) David Poirier 2026

********************************************************************************/


#ifdef ONENOTETORMDLL_EXPORTS
#define ONTR_API __declspec(dllexport)
#else
#define ONTR_API __declspec(dllimport)
#endif

typedef HANDLE HDOCFILE;
struct DrawDetailsParams {
	HDC hDC;
	RECT Rect;
};

#define PAGE_TYPE_WINDOW_RM_PAGE 1
#define PAGE_TYPE_TO_ONE_RM_PAGE 2
#define PAGE_TYPE_WINDOW_ONE_PAGE 3
#define PAGE_TYPE_TO_RM_ONE_PAGE 4

#define ERR_NEED_TOKEN -1
#define ERR_INVLAID_DOC_TYPE -2

extern "C" ONTR_API LPSTR GetIniFile();
extern "C" ONTR_API HRESULT GetIniFileB(LPCSTR Buffer, int BuffLen);
extern "C" ONTR_API void SetLogListbox(HWND hWnd);
extern "C" ONTR_API void DoLog(const char* Class, const char* Msg, int Level);

extern "C" ONTR_API void SetToken(const int PageType, LPCWSTR LoginCodeW);

extern "C" ONTR_API HRESULT ListDocs(const int PageType, LPCSTR Buffer, int BuffLen);

extern "C" ONTR_API HDOCFILE CreateEmptyDoc(const int PageType);
extern "C" ONTR_API int LoadDoc(HDOCFILE Doc, const char* FileName);
extern "C" ONTR_API HRESULT ConvertPage(HDOCFILE Source, HDOCFILE Dest, int Page);
extern "C" ONTR_API HRESULT ConvertPageB(HDOCFILE Source, DrawDetailsParams * DDP, int Page);
extern "C" ONTR_API time_t GetDocDateTime(HDOCFILE Source);
extern "C" ONTR_API int SaveDoc(HDOCFILE Doc, const char* FileName);
extern "C" ONTR_API HRESULT DeleteDoc(HDOCFILE Doc);

