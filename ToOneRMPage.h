#pragma once
#include "RMPage.h"
#include "WindowOnePage.h"
#include "ConversionConstants.h"

/*******************************************************************************

    ToOneRMPage.h

    Header for conversion from RM to OneNote

    This is a subclass of RMPage. It uses the RMPage subclass to load the RM page, 
        and then builds a OnePage that corresponds to it. The OnePage is added to 
        a GraphDoc that is created by its caller and passed in as the DrawDetails, 
        and that can then be used to save all the created pages.
    
    (C) David Poirier 2026

*******************************************************************************/

class ToOneRMPage : public RMPage
{
protected:
    virtual void DrawLineItem(void* DrawDetails, SceneLineItem* SIB);
    virtual void DrawTextItem(void* DrawDetails, RootText* RT);
    virtual void DrawPageInit(void* DrawDetails);

private:
    void FindMinMax(SceneLineItem* SLI);
    int GetRMFontSize(int format_code);
    std::wstring GetRMFont(int format_code);
    UINT32 paper_size[2] = { 100, 100 };
    Gdiplus::Rect LineExt = { 0, 0, 100, 100 };
    WindowONEPage OnePage;

public:
    ToOneRMPage();
    ToOneRMPage(std::string id);
    ~ToOneRMPage();
};

