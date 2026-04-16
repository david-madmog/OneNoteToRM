#include "framework.h"
#include "OneNoteToRM.h"

#include "RMPage.h"

#pragma pack( push, 1)
struct rm_frontmatter_header {
    char magic_text[32]; //         contents : reMarkable.lines file, version =
    char version_string; //        Version number, but encoded as a single UTF - 8 character.
    char ten_spaces[10]; //        contents : [0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20]
};

#pragma pack ( pop )

RMPage::RMPage()
{
}

RMPage::RMPage(std::string ID) {
    m_id = ID;
}

RMPage::~RMPage() {
    for (auto& Block : Blocks)
        delete Block;
    Blocks.clear();
}


void RMPage::Load(zip_file* file)
{
    zip_int64_t NumRead;
    int NumBlocks = 0;

    if (file) {
        // So, first we read the file header
        rm_frontmatter_header FM;
        NumRead = zip_fread(file, &FM, sizeof(FM));
//        DoLog((const char*)&FM);

        unsigned char * buffer;
        rm_BlockHead BH;

        // now read the rest as a series of blocks
        while (NumRead > 0) {
            // first read the block header, to see it's size and type
            NumRead = zip_fread(file, &BH, sizeof(BH));
            
            //buffer = (unsigned char *) malloc(BH.len_body);
            buffer = new unsigned char[BH.len_body];
            if (! buffer)
                NumRead = 0;

            // now the rest of the block
            if (NumRead > 0)
                NumRead = zip_fread(file, buffer, BH.len_body);
//            NumRead = zip_fread(file, buffer, 1024);
            if (NumRead != BH.len_body && NumRead > 0)
            {
                std::wostringstream LB;
                LB << L"Couldn't read the amount we wanted: Read " << NumRead << L" wanted " << BH.len_body;
                DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
            }


            if (NumRead > 0) {
                NumBlocks++;
                RMBlock* NewBlock = NULL;

                switch (BH.BlockType) {
                case BT_MigrationInfo:
                    NewBlock = new MigrationInfo();
                    break;
                case BT_SceneTree:
                    NewBlock = new SceneTree();
                    break;
                case BT_TreeNode:
                    NewBlock = new TreeNode();
                    break;
                case BT_SceneGlyphItem:
                    NewBlock = new SceneGlyphItem();
                    break;
                case BT_SceneGroupItem:
                    NewBlock = new SceneGroupItem();
                    break;
                case BT_SceneLineItem:
                    NewBlock = new SceneLineItem();
                    break;
                case BT_SceneTextItem:
                    NewBlock = new SceneTextItem();
                    break;
                case BT_RootText:
                    NewBlock = new RootText();
                    break;
                case BT_SceneTombstoneItem:
                    NewBlock = new SceneTombstoneItem();
                    break;
                case BT_AuthorIds:
                    NewBlock = new AuthorIds();
                    break;
                case BT_PageInfo:
                    NewBlock = new PageInfo();
                    break;
                case BT_SceneInfo:
                    NewBlock = new SceneInfo();
                    break;
                default:
                    std::wostringstream LB;
                    LB << L"Unknown block type " <<  BH.BlockType;
                    DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
                    break;
                }

                if (NewBlock) {
                    try {
                        bool bUseful = NewBlock->ParseBuffer(buffer, BH.len_body, BH.CurrentVersion);

                        // Stash the block, and also build it into the indexed collection if necessary
                        if (bUseful) {
                            AddBlock(NewBlock);
                        } else {
                            DoLog(typeid(*this).name(), "Useless block: ignoring", LOG_DEBUG_VERBOSE);
                        }
                    }
                    catch (incorrect_tag &e) {
                        std::wostringstream LB;
                        LB << L"Incorrect tag: " << e.what() << L" parsing block type " << typeid(*NewBlock).name();
                        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);
                    }
                }

            }

            if (buffer)
                delete[] buffer;
        }
        zip_fclose(file);
        std::wostringstream LB;
        LB << L"Read " << NumBlocks << L"blocks: " << Blocks.size() << L" blocks in Array";
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);
    }
    else {
        std::wostringstream LB;
        LB << L"Couldn't read the Zip File";
        DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);

    }
}

void* RMPage::Write(size_t *BuffSize)
{
    *BuffSize = sizeof(rm_frontmatter_header);

    for (auto& Block : Blocks)
    {
        *BuffSize += Block->PrepareWrite();
    }

    void * Buff = malloc(*BuffSize);

    void* Buff_Ptr = Buff;
    char FMH[] = "reMarkable .lines file, version=6          ";
    
    if (*BuffSize >= sizeof(rm_frontmatter_header) && Buff_Ptr)
    {
        memcpy(Buff_Ptr, (const void*)FMH, sizeof(rm_frontmatter_header));
        Buff_Ptr = (void*)((char*)Buff + sizeof(rm_frontmatter_header));
    }

    for (auto& Block : Blocks)
    {
        try
        {
            Buff_Ptr = Block->WriteBlock(Buff_Ptr);
        }
        catch (std::logic_error ex) {
            std::wostringstream LB;
            LB << L"Logic Error writing block of type " << typeid(*Block).name() << L" :" << ex.what();
            DoLog(typeid(*this).name(), LB.str(), LOG_ERROR);
        }
    }

    return Buff;
}

void RMPage::AddBlock(RMBlock * Block) {
    if (!Block)
        return;
    Blocks.push_back(Block);

    RM_CRDT_ID key;

    if (Block->BlockType() == BT_SceneTree)
        key = ((SceneTree*)Block)->tree_id;
    //    key = ((SceneTree*)Block)->node_id;
    else if (Block->BlockType() == BT_TreeNode)
        key = ((TreeNode*)Block)->node_id;
    else if (dynamic_cast<RMSceneItemBlock*>(Block))
        key = ((RMSceneItemBlock*)Block)->item_id;
    else if (Block->BlockType() == BT_RootText)
        if (((RootText*)Block)->texts.size() > 0)
            key = ((RootText*)Block)->texts[0].item_id;

    if (key != RM_CRDT_ID(0, 0))
    {
        if (IndexBlocks.count(key))
        {
            RMBlock * X = IndexBlocks[key];
            // Key exists already...
            std::wostringstream LB;
            LB << L"Key already exists. Key:" << key << L" Adding type " << Block->BlockType() << L", Existing type " << IndexBlocks[key]->BlockType();
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

        }
        IndexBlocks[key] = Block;
    }

}

void RMPage::DrawPage(void * DrawDetails) {
    DrawPageInit(DrawDetails);

    // We need to do two passes, so that we can be sure to have done the text anchors first
    for (auto const& [key, val] : IndexBlocks)
    {
        if (typeid(*val) == typeid(RootText))
        {
            std::wostringstream LB;
            LB << L"Drawing Text Block ID " << key ;
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
            DrawTextItem(DrawDetails, (RootText*)val);
        }
    }

    for (auto const& [key, val] : IndexBlocks)
    {
        if (typeid(*val) == typeid(SceneLineItem))
        {
            std::wostringstream LB;
            LB << L"Drawing Line Block ID " << key;
            DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
            DrawLineItem(DrawDetails, (SceneLineItem*)val);
        }
    }
}

void RMPage::DrawPageInit(void* DrawDetails)
{
    ; //Do nothing in base class
}

void RMPage::DrawLineItem(void* DrawDetails, SceneLineItem* SLI)
{
    ; //Do nothing in base class
}

void RMPage::DrawTextItem(void* DrawDetails, RootText* RT)
{
    ; //Do nothing in base class
}



std::wstring RMPage::DumpTree()
{
    std::wostringstream dump;

    for (auto const& [key, Block] : IndexBlocks)
    {
        dump << L"I:" << key;
        if (Block->BlockType() == BT_SceneTree)
            dump << L"SceneTree - TreeID:" << ((SceneTree*)Block)->tree_id << L" ParentID:" << ((SceneTree*)Block)->parent_id;
        else if (Block->BlockType() == BT_TreeNode)
            dump << L"TreeNode  - AnchorID:" << ((TreeNode*)Block)->anchor_id.value;
        else if (Block->BlockType() == BT_SceneGroupItem)
            dump << L"SceneItem - GroupItem ParentID:" << ((RMSceneItemBlock*)Block)->parent_id << L" ID5:" << ((SceneGroupItem*)Block)->ID5 ;
        else if (dynamic_cast<RMSceneItemBlock*>(Block))
            dump << L"SceneItem - SubblockType: " << ((RMSceneItemBlock*)Block)->SubBlockType() << L" ParentID:" << ((RMSceneItemBlock*)Block)->parent_id;
        else if (Block->BlockType() == BT_RootText)
            dump << L"RootText ";

        dump << std::endl;
    }
    for (auto const& [key, Point] : Anchors)
    {
        dump << L"A:" << key << L" [" << Point.x << L"," << Point.y << L"]" << std::endl;
    }

    std::map<RM_CRDT_ID, POINT> Anchors;


    return dump.str();
}
