#pragma once
#include <string>
#include <iostream>
#include "DOCXToRM.h"
#include "RMPage.h"
#include <wtypes.h>


class RMZipFile
{
private:
    const std::string WorkingDir = "C:\\Users\\david\\OneDrive\\Documents\\Development\\ReMarkable\\RMtoDOCX\\RMtoDOCX\\Working";
    std::vector<RMPage*> Pages;

public:
    RMZipFile();
    int ExtractRMsFromZip(const char* FileName);
    void DrawPage(HDC hDC, int page);
};
