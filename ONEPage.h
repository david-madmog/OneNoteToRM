#pragma once
#include <string>
#include "framework.h"
#include "OneNoteToRM.h"
#include <format>
#include <iostream>
#include <wtypes.h>
#include <tchar.h>
#include "GraphDoc.h"

class ONEPage
{
private:

public:
	void LoadPage(std::wstring * Data, std::string& Name);
	void DrawPage(void* DrawDetails);
};
