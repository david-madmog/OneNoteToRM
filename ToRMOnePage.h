#pragma once
#include "ONEPage.h"
#include "WindowRMPage.h"
#include "RMDocFile.h"

/*******************************************************************************

    ToRMOnePage.h

    Header for conversion from OneNote to RM 

    This is a subclass of ONEPage. It uses the ONBEPage subclass to load the page,
        and then builds an RMPage that corresponds to it. The RMPage is added to
        an RMDocFile that is created by its caller and passed in as the DrawDetails, 
        and that can then be used to save all the created pages.

    (C) David Poirier 2026

*******************************************************************************/

class ToRMOnePage : public ONEPage
{
private:
	WindowRMPage * RMPage;
	void LoadMetaData(RMDocFile<WindowRMPage> * DocFile);
    UINT8  DetermineRMFormatCode(ONE_TextSpan TextSpan);
    UINT32 DetermineRMColor(Gdiplus::Color InkColour);

public:
    ToRMOnePage();
	void DrawPage(void* DrawDetails);
};

