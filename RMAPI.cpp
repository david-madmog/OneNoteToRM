#include "pch.h"
#include "RMAPI.h"
#include "OneNoteToRM.h"
#include <shellapi.h>



using namespace std;

int RMAPI::filecopy(string from, string to)
{
    // Horrible... SHFileOperation needs "double null" terminator, and std::string keeps removing it
    char* cfrom = new char[from.size() + 2];
    char* cto = new char[to.size() + 2];
    memset(cfrom, 0, from.size() + 2);
    memset(cto, 0, to.size() + 2);
    memcpy_s(cfrom, from.size() + 2, from.c_str(), from.size());
    memcpy_s(cto, to.size() + 2, to.c_str(), to.size());

    SHFILEOPSTRUCTA FileOp = { 0 };
    FileOp.wFunc = FO_COPY;
    FileOp.pFrom = cfrom;  // can't use c_str() as that will destroy our terminating nulls
    FileOp.pTo = cto;
    FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION; // | FOF_NOERRORUI

    int ErrorCode = SHFileOperationA(&FileOp);
    return ErrorCode;
}


string RMAPI::exec(const char* cmd) {
// Source - https://stackoverflow.com/a/35658917
// Posted by TarmoPikaro, modified by community. See post 'Timeline' for change history
// Retrieved 2026-03-25, License - CC BY-SA 4.0
        string strResult;
        HANDLE hPipeRead, hPipeWrite;

        std::unique_ptr<char> TempDir(new char[LB_SIZE]);
        GetTempPathA(LB_SIZE - 1, TempDir.get());
//        GetPrivateProfileStringA("RMFILE", "WorkingDir", "", WorkingDir.get(), LB_SIZE, gszIniFileName);
       

        SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
        saAttr.bInheritHandle = TRUE; // Pipe handles are inherited by child process.
        saAttr.lpSecurityDescriptor = NULL;

        // Create a pipe to get results from child's stdout.
        if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
            return strResult;

        STARTUPINFOA si = { sizeof(STARTUPINFOA) };
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        si.hStdOutput = hPipeWrite;
        si.hStdError = hPipeWrite;
        si.wShowWindow = SW_HIDE; // Prevents cmd window from flashing.
        // Requires STARTF_USESHOWWINDOW in dwFlags.

        PROCESS_INFORMATION pi = { 0 };

        BOOL fSuccess = CreateProcessA(NULL, (LPSTR)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, (LPCSTR)TempDir.get(), &si, &pi);
        if (!fSuccess)
        {
            CloseHandle(hPipeWrite);
            CloseHandle(hPipeRead);
            return strResult;
        }

        bool bProcessEnded = false;
        for (; !bProcessEnded;)
        {
            // Give some timeslice (50 ms), so we won't waste 100% CPU.
            bProcessEnded = WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0;

            // Even if process exited - we continue reading, if
            // there is some data available over pipe.
            for (;;)
            {
                char buf[1024];
                DWORD dwRead = 0;
                DWORD dwAvail = 0;

                if (!::PeekNamedPipe(hPipeRead, NULL, 0, NULL, &dwAvail, NULL))
                    break;

                if (!dwAvail) // No data available, return
                    break;

                if (!::ReadFile(hPipeRead, buf, min(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
                    // Error, the child process might ended
                    break;

                buf[dwRead] = 0;
                strResult += buf;
            }
        } //for

        CloseHandle(hPipeWrite);
        CloseHandle(hPipeRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return strResult;
    } //ExecCmd





    //array<char, 128> buffer{};
    //string result;
    //unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    //if (!pipe) {
    //    throw runtime_error("popen() failed!");
    //}
    //while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
    //    result += buffer.data();
    //}
    //return result;
//}

void RMAPI::GetDoc(std::string Name)
{
	string Result = "";
    string Command = "";

    std::ranges::replace(Name, '\\', '/'); 

    std::unique_ptr<char> RMAPIDir(new char[LB_SIZE]);
    GetPrivateProfileStringA("RMFILE", "RMAPIDir", "", RMAPIDir.get(), LB_SIZE, gszIniFileName);
    Command.append(RMAPIDir.get());
    Command.append("rmapi get \"");
    Command.append(Name);
    Command.append("\"");

    std::wostringstream LB;
    LB << "Document get: " << Command.c_str();
    DoLog("RMAPI", LB.str(), LOG_DEBUG);

    Result = exec(Command.c_str());
	DoLog("RMAPI", Result.c_str(), LOG_DEBUG);
}

std::string RMAPI::ListDocsString()
{
    string Command = "";
    DoLog("RMAPI", "Querying RM API for doc list", LOG_DEBUG);

    std::unique_ptr<char> RMAPIDir(new char[LB_SIZE]);
    GetPrivateProfileStringA("RMFILE", "RMAPIDir", "", RMAPIDir.get(), LB_SIZE, gszIniFileName);
    Command.append(RMAPIDir.get());
    Command.append("rmapi find");
    stringstream result(exec(Command.c_str()));

    // Now trim down 
    vector<string> strings;
    string s;
    stringstream TrimmedResult;

    while (getline(result, s)) {
        // something like : [f] \General topics
        // or               [f] \\dir\file
        if (s[1] == 'f') {
            s.erase(0, 5);
            if (s[0] == '\\')
                s.erase(0, 1);
            TrimmedResult << s << endl;
        }
    }

    return TrimmedResult.str();
}
    
//void RMAPI::ListDocsStringToVector(std::string ListDocsString, vector<wstring>& Docs) {
//    wstringstream  wresult;
//    wresult << ListDocsString.c_str();
//
//    vector<string> strings;
//    wstring s;
//    while (getline(wresult, s)) {
//        // something like : [f] \General topics
//        // or               [f] \\dir\file
//        if (s[1] == 'f') {
//            s.erase(0, 5);
//            if (s[0] == '\\')
//                s.erase(0, 1);
//            Docs.push_back(s);
//        }
//    }
//}

void RMAPI::SaveDoc(std::string Name, std::string path) {
    // trash\T4
    
    //    std::string Zipfile = WorkingDir;
    std::string Zipfile = "";
    Zipfile.append(Name);
    Zipfile.append(".rmdoc");
    std::string TMPfile = path;
    TMPfile.append("Output.rmdoc");
    
    std::string TMPZfile = path;
    TMPZfile.append(Zipfile);

    // Horrible... SHFileOperation needs "double null" terminator
    int result = filecopy(TMPfile, TMPZfile);

    std::wostringstream LB;
    if (result != ERROR_SUCCESS)
    {
        LPTSTR errmessage;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, 0, (LPTSTR)&errmessage, 10,  NULL);
        LB << L"Document Copy status " << result << L" (" << errmessage << L")";
        DoLog("RMAPI", LB.str(), LOG_ERROR);
    }


    // put T4.rmdoc /trash --force
    std::string Command = "";
    std::unique_ptr<char> RMAPIDir(new char[LB_SIZE]);
    GetPrivateProfileStringA("RMFILE", "RMAPIDir", "", RMAPIDir.get(), LB_SIZE, gszIniFileName);
    Command.append(RMAPIDir.get());
    Command.append("rmapi put \"");
    Command.append(Zipfile);
    Command.append("\" ");
    //Command.append(path);
    Command.append(" --force");

    LB.str(L"");
    LB << "Document PUT: " << Command.c_str();
    DoLog("RMAPI", LB.str(), LOG_DEBUG);

    std::string Result = exec(Command.c_str());
    DoLog("RMAPI", Result.c_str(), LOG_DEBUG);
}

void RMAPI::CopyDoc(std::string Name) {
    //    std::string Zipfile = WorkingDir;
    std::string Zipfile = "";
    Zipfile.append(Name);
    Zipfile.append(".rmdoc");
    std::string TMPfile = "";
    TMPfile.append("Output.rmdoc");

    string Result = "";
    string Command = "";

    Command = "copy Output.rmdoc \"";
    Command.append(Zipfile);
    Command.append("\" /B /Y");
    Result = exec(Command.c_str());
    DoLog("RMAPI", Result.c_str(), LOG_DEBUG);
}

