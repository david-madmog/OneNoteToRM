#pragma once
#include <string>
#include "framework.h"
#include <format>
#include "DOCXToRM.h"
#include <iostream>
#include <wtypes.h>
#include <tchar.h>
#include "RMBlock.h"
#include "RMTree.h"
#include "RMBlockTypes.h"

class RMPage
{
private:
    std::vector<RMBlock*> Blocks;
    RMTree Tree;
    void DrawLineItem(HDC hDC, SceneLineItem* SIB);
    void FindMinMax(SceneLineItem* SLI);

    int MinX, MaxX, MinY, MaxY;

public:
    RMPage();
    void Load(zip_file* file);
    void DrawPage(HDC hDC);


};
