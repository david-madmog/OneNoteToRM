#include "pch.h"
#include "GraphDoc.h"
#include "OneNoteToRM.h"

/*******************************************************************************

    GraphDoc.cpp

    See header for documentation

    (C) David Poirier 2026

********************************************************************************/

#pragma warning ( push )
#pragma warning( disable : 26439 26495)
#include <cpprest/asyncrt_utils.h>
#pragma warning ( pop )



#pragma warning ( push )
#pragma warning( disable : 4005 26819)
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )

using njson = nlohmann::json;




// We need to include possible types so compiler knows what instantiations we need
#include "WindowONEPage.h"
template GraphDoc<WindowONEPage>::GraphDoc(GraphAPI* API);
template GraphDoc<WindowONEPage>::~GraphDoc();
template int GraphDoc<WindowONEPage>::LoadDoc(const wchar_t* SectionID);
template int GraphDoc<WindowONEPage>::LoadDoc(const std::string& NotebookName, const std::string& SectionName);
template int GraphDoc<WindowONEPage>::SaveDoc(const std::string& NotebookName, const std::string& SectionName);
template int GraphDoc<WindowONEPage>::SaveDoc(const wchar_t* SectionID);
template time_t GraphDoc<WindowONEPage>::LastEditTime();
template void GraphDoc<WindowONEPage>::DrawPage(void* DrawDetails, int Page);

#include "ToRMONEPage.h"
template GraphDoc<ToRMOnePage>::GraphDoc(GraphAPI* API);
template GraphDoc<ToRMOnePage>::~GraphDoc();
template int GraphDoc<ToRMOnePage>::LoadDoc(const wchar_t* SectionID);
template int GraphDoc<ToRMOnePage>::LoadDoc(const std::string& NotebookName, const std::string& SectionName);
template int GraphDoc<ToRMOnePage>::SaveDoc(const std::string& NotebookName, const std::string& SectionName);
template int GraphDoc<ToRMOnePage>::SaveDoc(const wchar_t* SectionID);
template time_t GraphDoc<ToRMOnePage>::LastEditTime();
template void GraphDoc<ToRMOnePage>::DrawPage(void* DrawDetails, int Page);


template<class PageType> GraphDoc<PageType>::GraphDoc(GraphAPI* pAPI) {
    API = pAPI;
}

template<class PageType> GraphDoc<PageType>::~GraphDoc() {
//    for (PageType* Page : Pages)
//        delete Page;
}


template<class PageType> int GraphDoc<PageType>::LoadDoc(const wchar_t * SectionID) {

    if (!API)
        return 0;

    if (!API->EnsureConnected())
        return -1;

    // First get the list of pages
    std::wstring PagesList = L"me/onenote/sections/";
    PagesList.append(SectionID);
    PagesList.append(L"/pages?$select=id,title,lastModifiedDateTime");

    std::wstring* RespData = API->SendRequestAndAwaitResponse(PagesList.c_str());
    if (RespData == nullptr)
        return 0;

    njson respJson;
    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return 0;
    }

    if (respJson.contains("value")) {
        size_t convertedChars = 0;
        wchar_t LocalWBuff[1024];

        // Now we have the list of pages, get the content for each one
		for (njson& PageJson : respJson["value"])
        {
            std::wstring PageQuery{ L"me/onenote/pages/" };
            std::string ID = PageJson["id"].get<std::string>();
            std::string LMDT = PageJson["lastModifiedDateTime"].get<std::string>();
            mbstowcs_s(&convertedChars, LocalWBuff, 1024, ID.c_str(), ID.length());
            PageQuery.append(LocalWBuff);
            PageQuery.append(L"/content?includeinkML=true");
            std::wstring* PageData = API->SendRequestAndAwaitResponse(PageQuery.c_str());
            if (PageData)
            {
                PageType* Page = new PageType;
                Pages.push_back(Page);
                std::string Title{ PageJson["title"].get<std::string>() };
                Page->LoadPage(PageData, Title);
                Page->LastMod = LMDT;
            }
        }
    }

    return (int)Pages.size();
}

template<class PageType> void GraphDoc<PageType>::DeletePages(const wchar_t* SectionID)
{
    // First get the list of pages
    std::wstring PagesList = L"me/onenote/sections/";
    PagesList.append(SectionID);
    PagesList.append(L"/pages?$select=id,title");

    std::wstring* RespData = API->SendRequestAndAwaitResponse(PagesList.c_str());
    if (RespData == nullptr)
        return;

    njson respJson;
    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return;
    }

    if (respJson.contains("value")) {
        size_t convertedChars = 0;
        wchar_t LocalWBuff[1024];

        // Now we have the list of pages, get the content for each one
        for (njson& PageJson : respJson["value"])
        {
            std::wstring PageQuery{ L"me/onenote/pages/" };
            std::string ID = PageJson["id"].get< std::string>();
            mbstowcs_s(&convertedChars, LocalWBuff, 1024, ID.c_str(), ID.length());
            PageQuery.append(LocalWBuff);
            API->DeletePage(PageQuery.c_str());
        }
    }
}

template<class PageType> wchar_t * GraphDoc<PageType>::FindDocID(const std::string& NotebookName,const std::string& SectionName)
{
    // Get full list of sections and get the ID of the one which matches our input
    std::wstring* RespData = API->SendRequestAndAwaitResponse(L"me/onenote/sections?$select=id,displayName&$expand=parentNotebook($select=id,displayName)");
    
    if (!RespData)
        return nullptr;

    njson respJson;
    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return 0;
    }

    if (respJson.contains("value")) {
        for (njson & Section : respJson["value"]) 
        {
            if (Section.contains("displayName") &&
                Section.contains("parentNotebook") &&
                Section["parentNotebook"].contains("displayName")
                ) 
            {
//                std::string XXX = Section.dump(4);
                std::string FoundSectionName = Section["displayName"].get< std::string>() ;
                std::string FoundNotebookName = Section["parentNotebook"]["displayName"].get< std::string>();

                if (SectionName== FoundSectionName && NotebookName==FoundNotebookName) {
                    // Hooray - found what we're looking for
                    std::string SectionID = Section["id"].get< std::string>();
                    size_t convertedChars = 0;
                    wchar_t* LocalWBuff = (wchar_t*)malloc((SectionID.length() + 1) * sizeof(wchar_t));
                    mbstowcs_s(&convertedChars, LocalWBuff, SectionID.length() + 1, SectionID.c_str(), SectionID.length());
                    return LocalWBuff;
                }
            }
        }

        // If we've got here, the section doesn't exist, so create it
        return CreateSection(NotebookName, SectionName);
    }


    return 0;
}

template<class PageType> wchar_t* GraphDoc<PageType>::CreateSection(const std::string& NotebookName, const std::string& SectionName)
{
    std::string NotebookID = "";

    //First we need to find the ID of the notebook...
    std::wstring* RespData = API->SendRequestAndAwaitResponse(L"me/onenote/notebooks?$select=id,displayName");

    if (!RespData)
        return nullptr;

    njson respJson;
    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return 0;
    }
    if (respJson.contains("value")) {
        for (njson& Section : respJson["value"])
        {
            if (Section.contains("displayName"))
            {
                //                std::string XXX = Section.dump(4);
                std::string FoundNotebookName = Section["displayName"].get< std::string>();

                if (NotebookName == FoundNotebookName) {
                    // Hooray - found what we're looking for
                    NotebookID = Section["id"].get< std::string>();
                    break;
                }
            }
        }
    }

    if (NotebookID.empty())
        return nullptr; // Couldn't find the notebook for some reason
    
    std::wstring URL = L"/me/onenote/notebooks/";
    URL.append(s2ws(NotebookID));
    URL.append(L"/sections");

    njson RequestJson;
    RequestJson.emplace("displayName", SectionName);
    std::wstring Body = s2ws(RequestJson.dump());

    RespData = API->PostUpdateAndAwaitResponse(URL.c_str(), Body.c_str(), NULL);

    if (!RespData)
        return nullptr;

    try {
        respJson = njson::parse(*RespData);
    }
    catch (njson::parse_error ex) {
        std::wostringstream LB;
        LB << L"JSON Parse Error: " << ex.what();
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        return 0;
    }
    if (respJson.contains("id")) {
        // Hooray - found what we're looking for
        std::string SectionID = respJson["id"].get< std::string>();
        size_t convertedChars = 0;
        wchar_t* LocalWBuff = (wchar_t*)malloc((SectionID.length() + 1) * sizeof(wchar_t));
        mbstowcs_s(&convertedChars, LocalWBuff, SectionID.length() + 1, SectionID.c_str(), SectionID.length());
        return LocalWBuff;
    }

    /*
POST https://graph.microsoft.com/v1.0/me/onenote/notebooks/{id}/sections
Content-type: application/json

{
  "displayName": "Section name"
}

HTTP/1.1 201 Created
Content-type: application/json

{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/notebooks('0-ADAEA281180757D1%21s21b15485600a4dc3911b99974f571c62')/sections/$entity",
    "id": "0-ADAEA281180757D1!s9bc9e7665956446e9c8a2149cea6adcd",
    "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s9bc9e7665956446e9c8a2149cea6adcd",
    "createdDateTime": "2026-04-14T12:22:54Z",
    "displayName": "Hello",
    "lastModifiedDateTime": "2026-04-14T12:22:54Z",
    "isDefault": false,
  "createdBy": {
    "user": {
      "id": "id-value",
      "displayName": "displayName-value"
    }
  },
  "lastModifiedBy": {
    "user": {
      "id": "id-value",
      "displayName": "displayName-value"
    }
  }
}

*/
    return nullptr;
}



template<class PageType> int GraphDoc<PageType>::LoadDoc(const std::string& NotebookName, const std::string& SectionName)
{
    if (!API)
        return 0;

    if (!API->EnsureConnected())
        return -1;

    wchar_t* SectionID = FindDocID(NotebookName, SectionName);
    if (SectionID)
    {
        int NumPages = LoadDoc(SectionID);
        free(SectionID);
        return NumPages;
    }
    return 0;
}

template<class PageType> int GraphDoc<PageType>::SaveDoc(const std::string& NotebookName, const std::string& SectionName)
{
    if (!API)
        return 0;

    if (!API->EnsureConnected())
        return -1;

    wchar_t* SectionID = FindDocID(NotebookName, SectionName);
    if (SectionID)
    {
        DeletePages(SectionID);

        utility::nonce_generator NonceGen;
        for (auto& Page : Pages) {
            utility::string_t Nonce = NonceGen.generate();

            std::wstring* PageData = Page->SavePage(Nonce);

            std::wstring PageURL = L"me/onenote/sections/";
            PageURL.append(SectionID);
            PageURL.append(L"/pages");

            std::wstring* RespData = API->PostUpdateAndAwaitResponse(PageURL.c_str(), PageData->c_str(), Nonce.c_str());
            if (RespData)
            {
                njson respJson;
                try {
                    respJson = njson::parse(*RespData);
                }
                catch (njson::parse_error ex) {
                    std::wostringstream LB;
                    LB << L"JSON Parse Error: " << ex.what();
                    DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
                    return 0;
                }
            }

            delete PageData;
        }

        //int NumPages = LoadDoc(SectionID);
        free(SectionID);
    }
    return 0;
}
template<class PageType> int GraphDoc<PageType>::SaveDoc(const wchar_t* SectionID)
{
    if (!API)
        return 0;

    if (!API->EnsureConnected())
        return -1;

    DeletePages(SectionID);

    utility::nonce_generator NonceGen;
    for (auto& Page : Pages) {
        utility::string_t Nonce = NonceGen.generate();

        std::wstring* PageData = Page->SavePage(Nonce);

        std::wstring PageURL = L"me/onenote/sections/";
        PageURL.append(SectionID);
        PageURL.append(L"/pages");

        std::wstring* RespData = API->PostUpdateAndAwaitResponse(PageURL.c_str(), PageData->c_str(), Nonce.c_str());
        if (RespData)
        {
            njson respJson;
            try {
                respJson = njson::parse(*RespData);
            }
            catch (njson::parse_error ex) {
                std::wostringstream LB;
                LB << L"JSON Parse Error: " << ex.what();
                DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
                return 0;
            }
        }

        delete PageData;
    }
    return 0;
}

template<class PageType> void GraphDoc<PageType>::DrawPage(void* DrawDetails, int Page)
{
    if (Page < Pages.size())
    {
        ONEPage* P = Pages[Page];
        P->DrawPage(DrawDetails);
    }
}

template<class PageType> time_t GraphDoc<PageType>::LastEditTime()
{
    time_t EditTime = 0;
    const std::string Format = "%Y-%m-%dT%H:%M:%S%Z"; // "2026-04-17T18:02:10Z"
//    struct tm tmStruct;

    for (auto page : Pages) {
        using namespace std::chrono;

        std::istringstream ss(page->LastMod);

        local_seconds tp;
        ss >> parse(Format, tp);
        auto tp_utc = current_zone()->to_sys(tp);

        time_t PageTime = tp_utc.time_since_epoch().count();

        if (PageTime > EditTime)
            EditTime = PageTime;
    }

    return EditTime;
}


/*
* 
* 
* 
* 
* 
me/onenote/sections?$select=id,displayName&$expand=parentNotebook($select=id,displayName)
List of sections RESPONSE
{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections(id,displayName,parentNotebook(id,displayName))",
    "value": [
        {
            "id": "0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "displayName": "New Section 1",
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/parentNotebook(id,displayName)/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
                "displayName": "TestNotebook 2"
            }
        },
        {
            "id": "0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0",
            "displayName": "New Section 1",
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21sc9911f43ef4e4e3381d49eb4214618d0')/parentNotebook(id,displayName)/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62",
                "displayName": "Test1"
            }
        },
        {
            "id": "0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61",
            "displayName": "New Section 1",
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s4cc1e448591a40c39821dd696b95ad61')/parentNotebook(id,displayName)/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
                "displayName": "test3"
            }
        }
    ]
}

    //https://graph.microsoft.com/v1.0/me/onenote/sections/0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df/pages?$select=id,title


{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/pages(id,title)",
    "value": [
        {
            "id": "0-89b0f965a4f4434ba7d479926c2c1f82!1-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "title": "Second page"
        },
        {
            "id": "0-5b84d480aca444a4bb0c96faf54de213!1-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "title": "Test Notebook 2"
        }
    ]
}




{   "@odata.context":"https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/notebooks", 
    "value" : [
        {   "id":"0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62", 
            "self" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62", 
            "createdDateTime" : "2026-02-10T16:26:33Z", 
            "displayName" : "Test1", 
            "lastModifiedDateTime" : "2026 - 02 - 10T16 : 26 : 33Z", 
            "isDefault" : false, 
            "userRole" : "Owner", 
            "isShared" : false, 
            "sectionsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62/sections", 
            "sectionGroupsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62/sectionGroups", 
            "createdBy" : {"user":{"id":"ADAEA281180757D1", "displayName" : "David Poirier"}}, 
            "lastModifiedBy" : {"user":{"id":"ADAEA281180757D1", "displayName" : "David Poirier"}}, 
            "links" : {
                "oneNoteClientUrl":{"href":"onenote:https://d.docs.live.net/adaea281180757d1/Documents/Development/ReMarkable/DOCXToRM/Test1"}, 
                "oneNoteWebUrl" : {"href":"https://onedrive.live.com/redir.aspx?resid=ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62&id=documents&page=edit&cid=adaea281180757d1"}
            }
        },
        {   "id":"0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
            "self" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
            "createdDateTime" : "2026-02-10T16:37:10Z",
            "displayName" : "test3",
            "lastModifiedDateTime" : "2026-02-10T16:37:10Z",
            "isDefault" : false,
            "userRole" : "Owner",
            "isShared" : false,
            "sectionsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c/sections",
            "sectionGroupsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c/sectionGroups",
            "createdBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "lastModifiedBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "links" : {
                "oneNoteClientUrl":{"href":"onenote:https://d.docs.live.net/adaea281180757d1/Documents/Development/ReMarkable/DOCXToRM/test3"},
                "oneNoteWebUrl" : {"href":"https://onedrive.live.com/redir.aspx?resid=ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c&id=documents&page=edit&cid=adaea281180757d1"}
            } 
        }, 
        {   "id":"0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
            "self" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
            "createdDateTime" : "2026-02-20T15:54:20Z",
            "displayName" : "TestNotebook 2",
            "lastModifiedDateTime" : "2026-02-20T15:54:20Z",
            "isDefault" : false,
            "userRole" : "Owner",
            "isShared" : false,
            "sectionsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3/sections",
            "sectionGroupsUrl" : "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3/sectionGroups",  
            "createdBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "lastModifiedBy" : {"user":{"id":"ADAEA281180757D1","displayName" : "David Poirier"}},
            "links" : {
                "oneNoteClientUrl":{"href":"onenote:https://d.docs.live.net/adaea281180757d1/Documents/TestNotebook 2"},
                "oneNoteWebUrl" : {"href":"https://onedrive.live.com/redir.aspx?resid=ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3&id=documents&page=edit&cid=adaea281180757d1"}
            } 
        }
    ] 
}


{
    "@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections",
    "@microsoft.graph.tips": "Use $select to choose only the properties your app needs, as this can lead to performance improvements. For example: GET me/onenote/sections?$select=isDefault,links",
    "value": [
        {
            "id": "0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df",
            "createdDateTime": "2026-02-20T15:55:30Z",
            "displayName": "New Section 1",
            "lastModifiedDateTime": "2026-02-20T15:55:33Z",
            "isDefault": false,
            "pagesUrl": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s1756d7d985564b62aff053397eb347df/pages",
            "createdBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "lastModifiedBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/parentNotebook/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3",
                "displayName": "TestNotebook 2",
                "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s1b2642be5dce4e2cb16bee5f157a4db3"
            },
            "parentSectionGroup@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s1756d7d985564b62aff053397eb347df')/parentSectionGroup/$entity",
            "parentSectionGroup": null
        },
        {
            "id": "0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0",
            "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0",
            "createdDateTime": "2026-02-10T16:26:46Z",
            "displayName": "New Section 1",
            "lastModifiedDateTime": "2026-02-10T16:28:09Z",
            "isDefault": false,
            "pagesUrl": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!sc9911f43ef4e4e3381d49eb4214618d0/pages",
            "createdBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "lastModifiedBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21sc9911f43ef4e4e3381d49eb4214618d0')/parentNotebook/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62",
                "displayName": "Test1",
                "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!s21b15485600a4dc3911b99974f571c62"
            },
            "parentSectionGroup@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21sc9911f43ef4e4e3381d49eb4214618d0')/parentSectionGroup/$entity",
            "parentSectionGroup": null
        },
        {
            "id": "0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61",
            "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61",
            "createdDateTime": "2026-02-20T15:55:19Z",
            "displayName": "New Section 1",
            "lastModifiedDateTime": "2026-02-20T15:55:23Z",
            "isDefault": false,
            "pagesUrl": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/sections/0-ADAEA281180757D1!s4cc1e448591a40c39821dd696b95ad61/pages",
            "createdBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "lastModifiedBy": {
                "user": {
                    "id": "ADAEA281180757D1",
                    "displayName": "David Poirier"
                }
            },
            "parentNotebook@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s4cc1e448591a40c39821dd696b95ad61')/parentNotebook/$entity",
            "parentNotebook": {
                "id": "0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c",
                "displayName": "test3",
                "self": "https://graph.microsoft.com/v1.0/users/david@madmog.co.uk/onenote/notebooks/0-ADAEA281180757D1!sd63d087487064ffeaeb5e452d32aae0c"
            },
            "parentSectionGroup@odata.context": "https://graph.microsoft.com/v1.0/$metadata#users('david%40madmog.co.uk')/onenote/sections('0-ADAEA281180757D1%21s4cc1e448591a40c39821dd696b95ad61')/parentSectionGroup/$entity",
            "parentSectionGroup": null
        }
    ]
}


*/