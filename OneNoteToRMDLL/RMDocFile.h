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

template<class PageType> class RMDocFile : public Drawable
{
private:
    RMAPI * API;

protected:
    void LoadMetaData(json J);
    void LoadContentData(json J);
    void LoadPageData(json J);
    void* WriteMetaData(zip* archive, const char* docID);
    void* WritePagesData(zip* archive, const char* docID);
    void* WritePage(zip* archive, const char* docID, RMPage* Page);

    void WriteZipData(zip* archive, const char* docID, const char* ext, void* data, size_t data_size);

    std::vector<PageType* > Pages;

    json LoadJSONData(zip_file* file, zip_uint64_t size);
public:
    RMDocFile(RMAPI * pAPI);
    ~RMDocFile();
//    int ExtractRMsFromZip(const char* FileName);
    int SaveRMsToZip(const char* FileName);

    int LoadDoc(std::wstring ID, std::wstring UUID);

    void DrawPage(void* DrawDetails, int page);
    void AddPage(PageType* Page) { Pages.push_back(Page); }
    time_t LastEditTime();

    json Metadata;
    std::string Name;
};

