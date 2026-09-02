#pragma once
#include <string>
#include <iostream>
#include "OneNoteToRM.h"
#include "RMPage.h"
#include "RMAPI.h"
#include <wtypes.h>
#pragma warning ( push )
#pragma warning( disable : 4005 26819)
//#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )
#include <type_traits> 

/*******************************************************************************

    RMDocFile.h

    Header for generic ReMarkable file format - i.e. a .rmdoc such as that retrieved
    from rmapi.

    This is a templated class, based on the subclass of RMPage depending on the type of
    rendering needed.
    Note, the .cpp file needs to include possible types of RMPage so compiler knows what 
    instantiations we need

    (C) David Poirier 2026

    Based on knowledge from:
    * Rick Lupton - https://github.com/ricklupton rmscene
    * ddvk - https://github.com/ddvk

*******************************************************************************/

using json = nlohmann::json;

template<class PageType> class RMDocFile : public BaseDoc
{
private:
    RMAPI * API;

protected:
    void LoadMetaData(json J);
    void LoadContentData(json J);
    void LoadPageData(json J);
    std::string WriteMetaData();
    std::string WriteContentData();
    std::string WritePageData();

    std::vector<PageType* > Pages;

public:
    RMDocFile(RMAPI * pAPI);
    ~RMDocFile();

    void AddPage(PageType* Page) { Pages.push_back(Page); }

    json Metadata;

    //Override base class
    int LoadDoc(const std::string& Part1, const std::string& Part2);
    int SaveDoc(const std::string& Part1, const std::string& Part2);
    int LoadDoc(const wchar_t* SectionID);
    int SaveDoc(const wchar_t* SectionID);
    void DrawPage(void* DrawDetails, int page);
    time_t LastEditTime();
};

