#pragma once
#include <string>
#include <vector>

/*******************************************************************************

	RMAPI.h

	Header for wrapping RM API calls for getting, putting and listing RM docs
	Basically just shells out to RMAPI executable do do the work

	see https://github.com/ddvk/rmapi for download and documentation on RMAPI

	(C) David Poirier 2026

********************************************************************************/


class RMAPI
{
private:
	static std::string exec(const char* cmd); 
	static int filecopy(std::string from, std::string to);

public:
	static void GetDoc(std::string Name);
	static void ListDocs(std::vector<std::wstring>&Docs);
	static void SaveDoc(std::string Name, std::string path);
	static void CopyDoc(std::string Name);
};

