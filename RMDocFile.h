#pragma once
#include <string>
#include <iostream>
#include "DOCXToRM.h"
#include "RMPage.h"
#include <wtypes.h>
#include <nlohmann/json.hpp>

/*******************************************************************************

    RMDocFile.H

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

template<class PageType> class RMDocFile
{
protected:
    void LoadMetaData(zip* archive, int index, size_t size);
    void LoadPagesData(zip* archive, int index, size_t size);
    void* WriteMetaData(zip* archive, const char* docID);
    void* WritePagesData(zip* archive, const char* docID);
    void* WritePage(zip* archive, const char* docID, RMPage* Page);

    void WriteZipData(zip* archive, const char* docID, const char* ext, void* data, size_t data_size);

    std::vector<PageType* > Pages;
    json Metadata;

    json LoadJSONData(zip_file* file, zip_uint64_t size);
public:
    RMDocFile() { static_assert(std::is_base_of<RMPage, PageType>::value, "Doc file must be based on PageType derived from RMPage"); }
    int ExtractRMsFromZip(const char* FileName);
    int SaveRMsToZip(const char* FileName);
    void DrawPage(void* DrawDetails, int page);
};
