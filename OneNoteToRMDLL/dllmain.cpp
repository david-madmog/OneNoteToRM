// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <ShlObj_core.h>
#include <filesystem>

extern std::wstring AppLocalDirectory;
void CreateAppLocalDirectory();

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateAppLocalDirectory();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void CreateAppLocalDirectory()
{
    wchar_t* wTokenIniPath;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &wTokenIniPath);
    AppLocalDirectory = wTokenIniPath;
    CoTaskMemFree(wTokenIniPath);
    AppLocalDirectory.append(L"\\OneNoteToRM");
    
    std::filesystem::create_directory(AppLocalDirectory);
}