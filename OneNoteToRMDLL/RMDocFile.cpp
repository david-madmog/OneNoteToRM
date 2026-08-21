#include "pch.h"

/*******************************************************************************

    RMDocFile.cpp

    See header for documentation

    (C) David Poirier 2026

********************************************************************************/


#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000

#include "RMDocFile.h"
using namespace std;

// We need to include possible types so compiler knows what instantiations we need
#include "WindowRMPage.h"
template int RMDocFile<WindowRMPage>::LoadDoc(std::wstring ID, std::wstring UUID);
//template int RMDocFile<WindowRMPage>::ExtractRMsFromZip(const char* FileName);
template int RMDocFile<WindowRMPage>::SaveRMsToZip(const char* FileName);
template void RMDocFile<WindowRMPage>::DrawPage(void* DrawDetails, int Page);
template time_t RMDocFile<WindowRMPage>::LastEditTime();
template RMDocFile<WindowRMPage>::RMDocFile(RMAPI* pAPI);
template RMDocFile<WindowRMPage>::~RMDocFile();

#include "ToOneRMPage.h"
template int RMDocFile<ToOneRMPage>::LoadDoc(std::wstring ID, std::wstring UUID);
//template int RMDocFile<ToOneRMPage>::ExtractRMsFromZip(const char* FileName);
template int RMDocFile<ToOneRMPage>::SaveRMsToZip(const char* FileName);
template void RMDocFile<ToOneRMPage>::DrawPage(void* DrawDetails, int Page);
template time_t RMDocFile<ToOneRMPage>::LastEditTime();
template RMDocFile<ToOneRMPage>::RMDocFile(RMAPI* pAPI);
template RMDocFile<ToOneRMPage>::~RMDocFile();

template<class PageType> RMDocFile<PageType>::RMDocFile(RMAPI * pAPI) 
{ 
    static_assert(std::is_base_of<RMPage, PageType>::value, "Doc file must be based on PageType derived from RMPage");
    API = pAPI;
}

template<class PageType> RMDocFile<PageType>::~RMDocFile() {
    for (PageType* Page : Pages)
        delete Page;
    Pages.clear();
}

template<class PageType> int RMDocFile<PageType>::LoadDoc(std::wstring ID, std::wstring UUID)
{ 
    UUID.append(L".docSchema");

    wstring* NodeData = API->GetDataStorage(ID.c_str(), UUID.c_str());
    if (!NodeData)
        return 0;

    // Now, loop through the catalog and get all the parts
    std::wstringstream ss(*NodeData);
    wstring line;
    while (getline(ss, line))
    {
        // The first field is the ID we want to query, the third id the UUID (including it's extension)
        //c81c44980d2a67b170cab01e8a617e14571d8cfa8015bc9b3c454ddb369e50bb:0:bd924c5d-1e0c-40af-a915-f1931a1c9524.content:0:59659

        size_t i1, i2, i3; // used for string splitting index
        i1 = line.find(L':');
        if (i1 != string::npos)
        {
            i2 = line.find(L':', i1 + 1);
            if (i2 != string::npos)
            {
                i3 = line.find(L':', i2 + 1);
                if (i3 != string::npos)
                {
                    wstring lID = line.substr(0, i1);
                    wstring lUUID = line.substr(i2 + 1, i3 - i2 - 1);
                    wstring ext = lUUID.substr(lUUID.find(L".") + 1, string::npos);

                    if (ext == L"rm")
                    {
                        std::wostringstream LB;
                        LB << L"Loading Page " << lUUID;
                        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

                        concurrency::streams::istream DocData = API->GetPage(lID.c_str(), lUUID.c_str());
                        // Find the relevant page
                        char* strtok_context_2 = NULL;
                        char* namesegment = strtok_s((char*)lUUID.c_str(), "/", &strtok_context_2);
                        char* lastnamesegment = namesegment;
                        while (namesegment) {
                            lastnamesegment = namesegment;
                            namesegment = strtok_s(NULL, "/", &strtok_context_2);
                        }
                        if (lastnamesegment == NULL)
                            continue;

                        std::string ID(lastnamesegment);
                        PageType* Page = new PageType(ID);
                        Pages.push_back(Page);
                        Page->Load(DocData);
                    }
                    else if (ext == L"metadata")
                    {
                        wstring* DocData = API->GetDataStorage(lID.c_str(), lUUID.c_str());
                        json J = json::parse(*DocData);
                        LoadMetaData(J);
                        delete DocData;
                    }
                    else if (ext == L"content")
                    {
                        wstring* DocData = API->GetDataStorage(lID.c_str(), lUUID.c_str());
                        json J = json::parse(*DocData);
                        LoadContentData(J);
                        delete DocData;
                    }
                    else if (ext == L"pagedata")
                        ;
                    else if (ext == L"pdf")
                    {
                        std::wostringstream LB;
                        LB << L"Unsupported file type in Catalog: " << ext;
                        DoLog(typeid(*this).name(), LB.str(), LOG_WARNING);
                    }
                    else
                    {
                        std::wostringstream LB;
                        LB << L"Unrecognised file type in Catalog: " << ext;
                        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
                    }
                    
                }
            }
        }
    }

    delete NodeData;
    DoLog(typeid(*this).name(), "Done Loading Document", LOG_DEBUG);

    int i = 1;
    for (auto& Page : Pages)
    {
        std::ostringstream PageName;
        //        PageName << Name << " - Page " << i++;
        PageName << "Page " << i++;
        Page->PageTitle = PageName.str();
    }

    return (int)Pages.size();
}


//template<class PageType> int RMDocFile<PageType>::ExtractRMsFromZip(const char* FileName)
//{
//    int err;
//    zip* archive = zip_open(FileName, ZIP_RDONLY, & err);
//    
//    if (!archive)
//    {
//        zip_error_t Zerr;
//        zip_error_init_with_code(&Zerr, err);
//        std::wostringstream LB;
//        LB << L"Unable to open rmdoc: error code " << err << L": " << zip_error_strerror(&Zerr);
//        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
//        zip_error_fini(&Zerr);
//        return 0;
//    }
//
//    // Step 2: Get the total number of files in the zip
//    // archive
//    zip_int64_t numFiles = zip_get_num_entries(archive, 0);
//
//    // Step 3: Loop through each file and print its contents
//    for (int ZipIndex = 0; ZipIndex < numFiles; ++ZipIndex) {
//        struct zip_stat fileInfo;
//        zip_stat_init(&fileInfo);
//
//        if (zip_stat_index(archive, ZipIndex, 0, &fileInfo) == 0) {
//            // see what sort of file it is, and if we want to try and parse it
//            DoLog(typeid(*this).name(), fileInfo.name, LOG_DEBUG);
//            char * strtok_context = NULL;
//            char* namebody = strtok_s((char*)fileInfo.name, ".", &strtok_context);
//            char* ext = strtok_s(NULL, ".", &strtok_context);
//                        
//            if (ext) {
//                if (!strcmp(ext, "rm"))
//                {
//                    // Find the relevant page
//                    char* strtok_context_2 = NULL;
//                    char* namesegment = strtok_s((char*)fileInfo.name, "/", &strtok_context_2);
//                    char* lastnamesegment = namesegment;
//                    while (namesegment) {
//                        lastnamesegment = namesegment;
//                        namesegment = strtok_s(NULL, "/", &strtok_context_2);
//                    }
//                    if (lastnamesegment == NULL)
//                        continue;
//
//                    std::string ID(lastnamesegment);
//                    bool bFound = false;
//                    for (PageType* Page : Pages)
//                    {
//                        if (*Page == ID) {
//                            Page->Load(zip_fopen_index(archive, ZipIndex, 0));
//                            bFound = true;
//                        }
//                    }
//                    if (!bFound) {
//                        PageType* Page = new PageType(ID);
//                        Pages.push_back(Page);
//                        Page->Load(zip_fopen_index(archive, ZipIndex, 0));
//                    }
//                }
//                else if (!strcmp(ext, "metadata"))
//                {
//                    json J = LoadJSONData(zip_fopen_index(archive, ZipIndex, 0), fileInfo.size);
//                    LoadMetaData(J);
//                }
//                else if (!strcmp(ext, "content"))
//                {
//                    json J = LoadJSONData(zip_fopen_index(archive, ZipIndex, 0), fileInfo.size);
//                    LoadContentData(J);
//                }
//                else if (!strcmp(ext, "pagedata"))
//                    ; // We have nothing to do with this for now... don't know what it is!
//                else {
//                    std::wostringstream LB;
//                    LB << L"Unrecognised file type in Zip: " << ext ;
//                    DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
//                }
//            }
//
//        }
//        else {
//            zip_error_t * Zerr = zip_get_error(archive);
////            zip_error_init_with_code(&Zerr, archive->);
//            std::wostringstream LB;
//            LB << L"Unable to read zip for file index " << ZipIndex << L": " << zip_error_strerror(Zerr);
//            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
// //           zip_error_fini(&Zerr);
//        }
//    }
//
//    // Close the zip archive
//    zip_close(archive);
//    DoLog(typeid(*this).name(), "Done extract from Zip", LOG_DEBUG);
//
//    int i = 1;
//    for (auto&Page : Pages) 
//    {
//        std::ostringstream PageName;
////        PageName << Name << " - Page " << i++;
//        PageName << "Page " << i++;
//        Page->PageTitle = PageName.str();
//    }
//
//    return (int)Pages.size();
//}

template<class PageType> void RMDocFile<PageType>::LoadMetaData(json J) {
    Metadata = J;

    //std::string meta{ Metadata.dump(2) };

    if (Metadata.contains("visibleName")) {
        Name = Metadata["visibleName"];
        std::wostringstream LB;
        LB << L"Metadata: Doc Name:" << Name.c_str();
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);
    }

    //.metadata
    //{
    //    "createdTime": "0",
    //        "lastModified" : "1768844812925",
    //        "lastOpened" : "1768844757552",
    //        "lastOpenedPage" : 1,
    //        "metadatamodified" : false,
    //        "modified" : false,
    //        "new" : false,
    //        "parent" : "",
    //        "pinned" : false,
    //        "source" : "",
    //        "type" : "DocumentType",
    //        "visibleName" : "Jobs"
    //}

}

template<class PageType> void RMDocFile<PageType>::LoadPageData(json J) {
    std::wostringstream LB;
    LB << L"PageData: Doc Name:" << Name.c_str();
    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);
}



template<class PageType> void RMDocFile<PageType>::LoadContentData(json J) {
    json Content = J;
    std::string S = Content.dump(2);

    vector<json> C_Pages;
    if (Content.contains("cPages")) {
        if (Content["cPages"].contains("pages")) {
            C_Pages = Content["cPages"]["pages"];
        }
    }
    else if (Content.contains("pages")) {
        C_Pages = Content["pages"];
    }
    std::wostringstream LB;
    LB << L"Content:" << C_Pages.size() << L" pages listed";
    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

    // Now create a page object for each one...
    // Actually, no need...
    //for (json C_Page : C_Pages) {
    //    std::string ID;
    //    if (C_Page.is_string()) {
    //        ID = C_Page.get<std::string>();
    //    }
    //    else {
    //        ID = C_Page["id"];
    //    }
    //    PageType* Page = new PageType(ID);
    //    Pages.push_back(Page);
    //    /*
    //    .content
    //    {
    //        "cPages": {
    //            "lastOpened": {
    //                "timestamp": "1:53",
    //                "value": "ae870d7b-2fc6-4e18-a68c-0ee550d03fff"
    //            },
    //            "original": {
    //                "timestamp": "0:0",
    //                "value": -1
    //            },
    //            "pages": [
    //                {
    //                    "id": "ae870d7b-2fc6-4e18-a68c-0ee550d03fff",
    //                    "idx": {
    //                        "timestamp": "1:2",
    //                        "value": "ba"
    //                    },
    //                    "modifed": "1768844812930",
    //                    "template": {
    //                        "timestamp": "1:1",
    //                        "value": "P Lines medium"
    //                    }
    //                },
    //                ...
    //                }
    //            ],
    //            "uuids": [
    //                {
    //                    "first": "1113a7c5-7407-55cd-90f7-564883646219",
    //                    "second": 3
    //                },
    //                {
    //                    "first": "6c71cdbb-788b-55b7-990c-a669f9828858",
    //                    "second": 2
    //                },
    //                {
    //                    "first": "976e8536-7ab6-5c9a-a040-d6cb6e455b4f",
    //                    "second": 1
    //                }
    //            ]
    //        },
    //        "coverPageNumber": 0,
    //        "customZoomCenterX": 0,
    //        "customZoomCenterY": 936,
    //        "customZoomOrientation": "portrait",
    //        "customZoomPageHeight": 1872,
    //        "customZoomPageWidth": 1404,
    //        "customZoomScale": 1,
    //        "documentMetadata": {
    //        },
    //        "extraMetadata": {
    //            "LastActiveTool": "primary",
    //            "LastBallpointColor": "Black",
    //            "LastBallpointSize": "2",
    //            "LastBallpointv2Color": "Black",
    //            "LastBallpointv2Size": "2",
    //            "LastCalligraphyColor": "Black",
    //            "LastCalligraphySize": "2",
    //            "LastClearPageColor": "Black",
    //            "LastClearPageSize": "2",
    //            "LastEraseSectionColor": "Black",
    //            "LastEraseSectionSize": "2",
    //            "LastEraserColor": "Black",
    //            "LastEraserSize": "2",
    //            "LastEraserTool": "Eraser",
    //            "LastFinelinerColor": "Black",
    //            "LastFinelinerSize": "2",
    //            "LastFinelinerv2Color": "Black",
    //            "LastFinelinerv2Size": "1",
    //            "LastHighlighterColor": "Black",
    //            "LastHighlighterSize": "2",
    //            "LastHighlighterv2Color": "HighlighterYellow",
    //            "LastHighlighterv2Size": "1",
    //            "LastMarkerColor": "Black",
    //            "LastMarkerSize": "2",
    //            "LastMarkerv2Color": "Blue",
    //            "LastMarkerv2Size": "3",
    //            "LastPaintbrushColor": "Black",
    //            "LastPaintbrushSize": "2",
    //            "LastPaintbrushv2Color": "Black",
    //            "LastPaintbrushv2Size": "2",
    //            "LastPen": "Finelinerv2",
    //            "LastPencilColor": "Black",
    //            "LastPencilSize": "2",
    //            "LastPencilv2Color": "Black",
    //            "LastPencilv2Size": "2",
    //            "LastReservedPenColor": "Black",
    //            "LastReservedPenSize": "2",
    //            "LastSelectionToolColor": "Black",
    //            "LastSelectionToolSize": "2",
    //            "LastShadingMarkerColor": "ArgbCode",
    //            "LastShadingMarkerColorCode": "1075912220",
    //            "LastShadingMarkerSize": "2",
    //            "LastSharpPencilColor": "Black",
    //            "LastSharpPencilSize": "2",
    //            "LastSharpPencilv2Color": "Black",
    //            "LastSharpPencilv2Size": "2",
    //            "LastSolidPenColor": "Black",
    //            "LastSolidPenSize": "2",
    //            "LastZoomToolColor": "Black",
    //            "LastZoomToolSize": "2",
    //            "SecondaryHighlighterv2Color": "ArgbCode",
    //            "SecondaryHighlighterv2ColorCode": "4294962549",
    //            "SecondaryHighlighterv2Size": "1",
    //            "SecondaryPen": "Highlighterv2"
    //        },
    //        "fileType": "notebook",
    //        "fontName": "",
    //        "formatVersion": 2,
    //        "lineHeight": -1,
    //        "orientation": "portrait",
    //        "pageCount": 7,
    //        "pageTags": [
    //        ],
    //        "sizeInBytes": "644972",
    //        "tags": [
    //        ],
    //        "textAlignment": "justify",
    //        "textScale": 1,
    //        "zoomMode": "bestFit"
    //    }

    //    */
    //}
}


//template<class PageType> json RMDocFile<PageType>::LoadJSONData(zip_file* file, zip_uint64_t size)
//{
//    zip_int64_t NumRead;
//    json obj;
//    if (file) {
//        char* Buffer = (char*)calloc(size + 1, sizeof(char));
//
//        if (Buffer)
//        {
//            NumRead = zip_fread(file, Buffer, size);
//
//            try {
//                obj = json::parse(std::string(Buffer));
//            }
//            catch (exception ex)
//            {
//                DoLog(typeid(*this).name(), ex.what(), LOG_ERROR);
//            }
//        }
//    }
//    return obj;
//}




////////////////////////////////////////////////////////////////////////////////////////

template<class PageType> int RMDocFile<PageType>::SaveRMsToZip(const char* FileName)
{
    zip_error_t Zerr;
    int err;
    zip* archive = zip_open(FileName, ZIP_CREATE | ZIP_TRUNCATE, &err);
    

    UUID uuid;
    if (UuidCreate(&uuid) != RPC_S_OK)
        return 0;
    char* docID = nullptr;
    if( UuidToStringA(&uuid, (RPC_CSTR*)&docID) != RPC_S_OK)
        return 0;

    if (!archive)
    {
        zip_error_t Zerr;
        zip_error_init_with_code(&Zerr, err);
        std::wostringstream LB;
        LB << L"Unable to open rmdoc: error code " << err << L": " << zip_error_strerror(&Zerr);
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        zip_error_fini(&Zerr);
        return 0;
    }

    zip_set_archive_flag(archive, ZIP_AFL_CREATE_OR_KEEP_FILE_FOR_EMPTY_ARCHIVE, TRUE);
    // Buffer of data has to exist until we close the file, so we hold the memory refernces for now and free it later
    vector<void*> Buffers;
    Buffers.push_back(WriteMetaData(archive, (const char*)docID));
    Buffers.push_back(WritePagesData(archive, (const char*)docID));


    for (auto Page : Pages) {
        Buffers.push_back(WritePage(archive, (const char*)docID, Page));
    }

    // Close the zip archive
    if (zip_close(archive))
    {
        Zerr = * zip_get_error(archive);
        std::wostringstream LB;
        LB << L"Unable to open rmdoc: error code " << Zerr.zip_err << L": " << zip_error_strerror(&Zerr);
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
    }
    DoLog(typeid(*this).name(), "Done Write to Zip", LOG_DEBUG);

    RpcStringFreeA((RPC_CSTR*)&docID);
    for (void* Buff : Buffers)
        free(Buff);

    return (int)Pages.size();

}

template<class PageType> void * RMDocFile<PageType>::WriteMetaData(zip* archive, const char * docID) {

    Metadata["visibleName"] = Name;
    std::string MD{ Metadata.dump() };

    void* Buff = malloc(MD.size() + 1);
    if (Buff)
    {
        strcpy_s((char*)Buff, MD.size() + 1, MD.c_str());
        WriteZipData(archive, docID, ".metadata", Buff, MD.size());
    }
    return Buff;
}

struct JsonPageEntry {
    std::string id;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonPageEntry, id)

template<class PageType> void* RMDocFile<PageType>::WritePagesData(zip* archive, const char* docID) {
    std::vector<JsonPageEntry> JsonPageEntries;

    for (auto Page : Pages) {
        JsonPageEntry Entry{ Page->m_id };
        JsonPageEntries.push_back(Entry);
    }

    json content{ { "cPages", {{"pages", JsonPageEntries}}} };
    std::string CN{ content.dump() };
    void* Buff = malloc(CN.size() + 1);
    strcpy_s((char*)Buff, CN.size()+1, CN.c_str());
    WriteZipData(archive, docID, ".content", Buff, CN.size());
    return Buff;
}

template<class PageType> void * RMDocFile<PageType>::WritePage(zip* archive, const char* docID, RMPage * Page) {
    size_t BuffSize;
    void* PageBuffer = Page->Write(&BuffSize);

    size_t FNSize = strlen(docID) + strlen(Page->m_id.c_str()) +2;
    char* FileName = (char*)malloc(FNSize);
    if (!FileName)
        return NULL; 

    strcpy_s(FileName, FNSize, docID);
    strcat_s(FileName, FNSize, "/");
    strcat_s(FileName, FNSize, Page->m_id.c_str());

    WriteZipData(archive, FileName, ".rm", (void*)PageBuffer, BuffSize);

    return PageBuffer;
}

template<class PageType> void RMDocFile<PageType>::WriteZipData(zip* archive, const char* docID, const char* ext, void* data, size_t data_size) {

    size_t FNSize = strlen(docID) + strlen(ext) + 1;
    char* FileName = (char*)malloc(FNSize);
    if (!FileName)
        return;

    strcpy_s(FileName, FNSize, docID);
    strcat_s(FileName, FNSize, ext);

    zip_source_t* source = zip_source_buffer(archive, data, data_size, 0);

    if (source) {
        if (zip_file_add(archive, FileName, source, ZIP_FL_ENC_UTF_8) == -1) {
            zip_error_t Zerr = *zip_get_error(archive);
            std::wostringstream LB;
            LB << L"Unable to open rmdoc: error code " << Zerr.zip_err << L": " << zip_error_strerror(&Zerr);
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
            zip_source_free(source);
        }
    }
}


template<class PageType> void RMDocFile<PageType>::DrawPage(void* DrawDetails, int Page)
{
    if (Page < Pages.size())
    {
        RMPage* P = Pages[Page];
        P->DrawPage(DrawDetails);

#if 0
        std::wstring str = P->DumpTree();
        OutputDebugString(str.c_str());
#endif
    }
}


template<class PageType> time_t RMDocFile<PageType>::LastEditTime() {
    time_t EditTime = 0;
    std::string ETString;

    if (Metadata.contains("lastModified")) {
        ETString = Metadata["lastModified"];
        EditTime = std::stoull(ETString, nullptr) / 1000 ; // Convert from ms to standatd time_t
    }

    return EditTime;
}
