#pragma once
#include "RMPage.h"
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

};

