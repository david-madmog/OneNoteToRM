#pragma once
#include <string>
#include "framework.h"
#include "DOCXToRM.h"
#include <format>
#include <iostream>
#include <wtypes.h>
#include <tchar.h>
#include "GraphDoc.h"

class ONEPage
{
private:

public:
	void LoadPage(nlohmann::json PageJson);
	void DrawPage(void* DrawDetails);
};
