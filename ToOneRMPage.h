#pragma once
#include "RMPage.h"
#include "WindowOnePage.h"

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
    
    int MinX, MaxX, MinY, MaxY;

    WindowONEPage OnePage;

public:
    ToOneRMPage();
    ToOneRMPage(std::string id);
};

