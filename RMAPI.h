#pragma once
#include "framework.h"


class RMAPI
{
private:
	static std::string exec(const char* cmd); 

public:
	static void GetDoc(std::string Name);
	static void ListDocs(std::vector<std::wstring>&Docs);

};

