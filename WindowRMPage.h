#pragma once
#include "RMPage.h"

/*******************************************************************************

    WindowRMPage.h

    Header for display of an RMPage

    This is a subclass of RMPage. It uses the RMPage subclass to load the page,
        and then renders it to the DC passed in as part of the DrawDetails

    (C) David Poirier 2026

*******************************************************************************/

class WindowRMPage :
    public RMPage
{
protected:
    virtual void DrawLineItem(void* DrawDetails, SceneLineItem* SIB);
    virtual void DrawTextItem(void* DrawDetails, RootText* RT);
    virtual void DrawPageInit(void* DrawDetails);

private:
    Gdiplus::Font* GetRMFont(int format_code);
    void FindMinMax(SceneLineItem* SLI);
    int MinX, MaxX, MinY, MaxY;

public:
    WindowRMPage() ;
    WindowRMPage(std::string id) ;
    ~WindowRMPage();
};

