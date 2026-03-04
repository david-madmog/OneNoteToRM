#pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
#include "RMDocFile.h"
using namespace std;

// We need to include possible types so compiler knows what instantiations we need
#include "WindowRMPage.h"
template int RMDocFile<WindowRMPage>::ExtractRMsFromZip(const char* FileName);
template int RMDocFile<WindowRMPage>::SaveRMsToZip(const char* FileName);
template void RMDocFile<WindowRMPage>::DrawPage(void* DrawDetails, int Page);

#include "ToOneRMPage.h"
template int RMDocFile<ToOneRMPage>::ExtractRMsFromZip(const char* FileName);
template int RMDocFile<ToOneRMPage>::SaveRMsToZip(const char* FileName);
template void RMDocFile<ToOneRMPage>::DrawPage(void* DrawDetails, int Page);


template<class PageType> int RMDocFile<PageType>::ExtractRMsFromZip(const char* FileName)
{
    int err;
    zip* archive = zip_open(FileName, ZIP_RDONLY, & err);
    
    if (!archive)
    {
        zip_error_t Zerr;
        zip_error_init_with_code(&Zerr, err);
        sprintf_s(LogBuffer, LB_SIZE, "Unable to open rmdoc: error code %d (%s)", err, zip_error_strerror(&Zerr));
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        zip_error_fini(&Zerr);
        return 0;
    }

    // Step 2: Get the total number of files in the zip
    // archive
    zip_int64_t numFiles = zip_get_num_entries(archive, 0);

    // Step 3: Loop through each file and print its contents
    for (int ZipIndex = 0; ZipIndex < numFiles; ++ZipIndex) {
        struct zip_stat fileInfo;
        zip_stat_init(&fileInfo);

        if (zip_stat_index(archive, ZipIndex, 0, &fileInfo) == 0) {
            // see what sort of file it is, and if we want to try and parse it
            DoLog(typeid(*this).name(), fileInfo.name, LOG_DEBUG);
            char * strtok_context = NULL;
            char* namebody = strtok_s((char*)fileInfo.name, ".", &strtok_context);
            char* ext = strtok_s(NULL, ".", &strtok_context);
                        
            if (ext) {
                if (!strcmp(ext, "rm")) 
                {
                    // Find the relevant page
                    char* strtok_context_2 = NULL;
                    char* namesegment = strtok_s((char*)fileInfo.name, "/", &strtok_context_2);
                    char* lastnamesegment = namesegment;
                    while (namesegment) {
                        lastnamesegment = namesegment;
                        namesegment = strtok_s(NULL, "/", &strtok_context_2);
                    }
                    if (lastnamesegment == NULL)
                        continue;

                    std::string ID(lastnamesegment);
                    bool bFound = false;
                    for (PageType* Page: Pages)
                    {
                        if (*Page == ID) {
                            Page->Load(zip_fopen_index(archive, ZipIndex, 0));
                            bFound = true;
                        }
                    }
                    if (!bFound) {
                        PageType* Page = new PageType(ID);
                        Pages.push_back(Page);
                        Page->Load(zip_fopen_index(archive, ZipIndex, 0));

                    }
                }
                else if (!strcmp(ext, "metadata"))
                    LoadMetaData(archive, ZipIndex, fileInfo.size);
                else if (!strcmp(ext, "content"))
                    LoadPagesData(archive, ZipIndex, fileInfo.size);
                else {
                    sprintf_s(LogBuffer, LB_SIZE, "Unrecognised file type in Zip: %s", ext);
                    DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
                }
            }

        }
    }

    // Close the zip archive
    zip_close(archive);
    DoLog(typeid(*this).name(), "Done extract from Zip", LOG_DEBUG);

    return (int)Pages.size();
}


template<class PageType> void RMDocFile<PageType>::LoadMetaData(zip * archive, int index, size_t size) {
    Metadata = LoadJSONData(zip_fopen_index(archive, index, 0), size);
    sprintf_s(LogBuffer, LB_SIZE, "Metadata: Doc Name '%s'", Metadata["visibleName"].get<std::string>().c_str());
    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);

    std::string meta{ Metadata.dump(2) };


    //.metadata
    //{
    //    "createdTime": "1763459978040",
    //        "lastModified" : "1768844812925",
    //        "lastOpened" : "1768844757552",
    //        "lastOpenedPage" : 0,
    //        "new" : false,
    //        "parent" : "",
    //        "pinned" : false,
    //        "source" : "",
    //        "type" : "DocumentType",
    //        "visibleName" : "Jobs"
    //}

}

template<class PageType> void RMDocFile<PageType>::LoadPagesData(zip* archive, int index, size_t size) {
    json Content = LoadJSONData(zip_fopen_index(archive, index, 0), size);
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
    sprintf_s(LogBuffer, LB_SIZE, "Content: %zd pages listed", C_Pages.size());
    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);

    // Now create a page object for each one...
    for (json C_Page : C_Pages) {
        std::string ID;
        if (C_Page.is_string()) {
            ID = C_Page.get<std::string>();
        }
        else {
            ID = C_Page["id"];
        }


        PageType* Page = new PageType(ID);
        Pages.push_back(Page);
        /*
        .content
        {
            "cPages": {
                "lastOpened": {
                    "timestamp": "1:53",
                    "value": "ae870d7b-2fc6-4e18-a68c-0ee550d03fff"
                },
                "original": {
                    "timestamp": "0:0",
                    "value": -1
                },
                "pages": [
                    {
                        "id": "ae870d7b-2fc6-4e18-a68c-0ee550d03fff",
                        "idx": {
                            "timestamp": "1:2",
                            "value": "ba"
                        },
                        "modifed": "1768844812930",
                        "template": {
                            "timestamp": "1:1",
                            "value": "P Lines medium"
                        }
                    },
                    ...
                    }
                ],
                "uuids": [
                    {
                        "first": "1113a7c5-7407-55cd-90f7-564883646219",
                        "second": 3
                    },
                    {
                        "first": "6c71cdbb-788b-55b7-990c-a669f9828858",
                        "second": 2
                    },
                    {
                        "first": "976e8536-7ab6-5c9a-a040-d6cb6e455b4f",
                        "second": 1
                    }
                ]
            },
            "coverPageNumber": 0,
            "customZoomCenterX": 0,
            "customZoomCenterY": 936,
            "customZoomOrientation": "portrait",
            "customZoomPageHeight": 1872,
            "customZoomPageWidth": 1404,
            "customZoomScale": 1,
            "documentMetadata": {
            },
            "extraMetadata": {
                "LastActiveTool": "primary",
                "LastBallpointColor": "Black",
                "LastBallpointSize": "2",
                "LastBallpointv2Color": "Black",
                "LastBallpointv2Size": "2",
                "LastCalligraphyColor": "Black",
                "LastCalligraphySize": "2",
                "LastClearPageColor": "Black",
                "LastClearPageSize": "2",
                "LastEraseSectionColor": "Black",
                "LastEraseSectionSize": "2",
                "LastEraserColor": "Black",
                "LastEraserSize": "2",
                "LastEraserTool": "Eraser",
                "LastFinelinerColor": "Black",
                "LastFinelinerSize": "2",
                "LastFinelinerv2Color": "Black",
                "LastFinelinerv2Size": "1",
                "LastHighlighterColor": "Black",
                "LastHighlighterSize": "2",
                "LastHighlighterv2Color": "HighlighterYellow",
                "LastHighlighterv2Size": "1",
                "LastMarkerColor": "Black",
                "LastMarkerSize": "2",
                "LastMarkerv2Color": "Blue",
                "LastMarkerv2Size": "3",
                "LastPaintbrushColor": "Black",
                "LastPaintbrushSize": "2",
                "LastPaintbrushv2Color": "Black",
                "LastPaintbrushv2Size": "2",
                "LastPen": "Finelinerv2",
                "LastPencilColor": "Black",
                "LastPencilSize": "2",
                "LastPencilv2Color": "Black",
                "LastPencilv2Size": "2",
                "LastReservedPenColor": "Black",
                "LastReservedPenSize": "2",
                "LastSelectionToolColor": "Black",
                "LastSelectionToolSize": "2",
                "LastShadingMarkerColor": "ArgbCode",
                "LastShadingMarkerColorCode": "1075912220",
                "LastShadingMarkerSize": "2",
                "LastSharpPencilColor": "Black",
                "LastSharpPencilSize": "2",
                "LastSharpPencilv2Color": "Black",
                "LastSharpPencilv2Size": "2",
                "LastSolidPenColor": "Black",
                "LastSolidPenSize": "2",
                "LastZoomToolColor": "Black",
                "LastZoomToolSize": "2",
                "SecondaryHighlighterv2Color": "ArgbCode",
                "SecondaryHighlighterv2ColorCode": "4294962549",
                "SecondaryHighlighterv2Size": "1",
                "SecondaryPen": "Highlighterv2"
            },
            "fileType": "notebook",
            "fontName": "",
            "formatVersion": 2,
            "lineHeight": -1,
            "orientation": "portrait",
            "pageCount": 7,
            "pageTags": [
            ],
            "sizeInBytes": "644972",
            "tags": [
            ],
            "textAlignment": "justify",
            "textScale": 1,
            "zoomMode": "bestFit"
        }

        */
    }
}


template<class PageType> json RMDocFile<PageType>::LoadJSONData(zip_file* file, zip_uint64_t size)
{
    zip_int64_t NumRead;
    json obj;
    if (file) {
        char* Buffer = (char*)calloc(size + 1, sizeof(char));

        if (Buffer)
        {
            NumRead = zip_fread(file, Buffer, size);

            try {
                obj = json::parse(std::string(Buffer));
            }
            catch (exception ex)
            {
                DoLog(typeid(*this).name(), ex.what(), LOG_ERROR);
            }
        }
    }
    return obj;
}

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
        sprintf_s(LogBuffer, LB_SIZE, "Unable to open rmdoc: error code %d (%s)", err, zip_error_strerror(&Zerr));
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
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
        sprintf_s(LogBuffer, LB_SIZE, "Unable to write rmdoc: error code %d (%s)", Zerr.zip_err, zip_error_strerror(&Zerr));
        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
    }
    DoLog(typeid(*this).name(), "Done Write to Zip", LOG_DEBUG);

    RpcStringFreeA((RPC_CSTR*)&docID);
    for (void* Buff : Buffers)
        free(Buff);

    return (int)Pages.size();

}

template<class PageType> void * RMDocFile<PageType>::WriteMetaData(zip* archive, const char * docID) {
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
            sprintf_s(LogBuffer, LB_SIZE, "Unable to write rmdoc: error code %d (%s)", Zerr.zip_err, zip_error_strerror(&Zerr));
            DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
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
    }
}

