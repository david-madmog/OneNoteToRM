#include "framework.h"
#include "RMAPI.h"
#include "OneNoteToRM.h"

using namespace std;

string RMAPI::exec(const char* cmd) {
    array<char, 128> buffer{};
    string result;
    unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void RMAPI::GetDoc(std::string Name)
{
	string Result = "";
    string Command = "";

    std::ranges::replace(Name, '\\', '/'); 
    Command = "rmapi get \"";

    Command.append(Name);
    Command.append("\"");

    std::wostringstream LB;
    LB << "Document get: " << Command.c_str();
    DoLog("RMAPI", LB.str().c_str(), LOG_DEBUG);

    Result = exec(Command.c_str());
	DoLog("RMAPI", Result.c_str(), LOG_DEBUG);
}

void RMAPI::ListDocs(vector<wstring>&Docs) {
    DoLog("RMAPI", "Querying RM API for doc list", LOG_DEBUG);

    string result = exec("rmapi find");
    wstringstream  wresult;
    wresult << result.c_str();

    vector<string> strings;
    wstring s;
    while (getline(wresult, s)) {
        // something like : [f] \General topics
        // or               [f] \\dir\file
        if (s[1] == 'f') {
            s.erase(0, 5);
            if (s[0] == '\\')
                s.erase(0, 1);
            Docs.push_back(s);
        }
    }
}

void RMAPI::SaveDoc(std::string Name) {
    // trash\T4
    
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

    // put T4.rmdoc /trash --force
    Command = "rmapi put \"";
    Command.append(Zipfile);
    Command.append("\" ");
    //Command.append(path);
    Command.append(" --force");

    std::wostringstream LB;
    LB << "Document PUT: " << Command.c_str();
    DoLog("RMAPI", LB.str().c_str(), LOG_DEBUG);

    Result = exec(Command.c_str());
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

