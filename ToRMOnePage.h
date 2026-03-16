#pragma once
#include "ONEPage.h"
#include "WindowRMPage.h"
#include "RMDocFile.h"
#include "framework.h"

class ToRMOnePage : public ONEPage
{
private:
	WindowRMPage RMPage;
	void LoadMetaData(RMDocFile<WindowRMPage> * DocFile);

public:
	void DrawPage(void* DrawDetails);
};

