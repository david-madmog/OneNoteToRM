#pragma once
#include "RMBlock.h"
#include "RMBlockTypes.h"
#include <cpprest/filestream.h>

#include <vector>
#include <map>

/*******************************************************************************

    RMPage.h

    Header for generic ReMarkable file format Pages
    Intent is for this class to be subclassed, with three virtual functions being 
    overridden for context specific rendering.

    (C) David Poirier 2026

    Based on knowledge from:
    * Rick Lupton - https://github.com/ricklupton rmscene
    * ddvk - https://github.com/ddvk

*******************************************************************************/


class RMPage
{
protected:
    std::vector<RMBlock*> Blocks;
    std::map<RM_CRDT_ID, RMBlock*> IndexBlocks;
    std::map<RM_CRDT_ID, POINT> Anchors;

    virtual void DrawLineItem(void* DrawDetails, SceneLineItem* SIB);
    virtual void DrawTextItem(void* DrawDetails, RootText* RT);
    virtual void DrawPageInit(void* DrawDetails);

public:
    RMPage();
    RMPage(std::string id);
    virtual ~RMPage();
    bool operator== (const RMPage& b) { return m_id == b.m_id; };
    bool operator== (const std::string& b) { return m_id == b; };
    bool operator< (const RMPage& b) { return m_id < b.m_id; };
    void Load(concurrency::streams::istream Doc);
    void * Write(size_t* BuffSize);
    void DrawPage(void * DrawDetails);
    void AddBlock(RMBlock * Block);
    std::string m_id;
    std::string PageTitle = "";

    std::wstring DumpTree();

    friend class ToRMOnePage;
};

inline bool RMPagePointerComp(RMPage* a, RMPage* b) { return (*a < *b); }