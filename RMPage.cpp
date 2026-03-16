
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
                sprintf_s(LogBuffer, LB_SIZE, "Couldn't read the amount we wanted: Read %d wanted %d", (int)NumRead, BH.len_body);
                DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
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
                    sprintf_s(LogBuffer, LB_SIZE, "**** Unknown Type %d len %d", BH.BlockType, BH.len_body);
                    DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
                    break;
                }

                if (NewBlock) {
                    try {
                        bool bUseful = NewBlock->ParseBuffer(buffer, BH.len_body, BH.CurrentVersion);

                        // Stash the block, and also build it into the indexed collection if necessary
                        if (bUseful) {
                            AddBlock(NewBlock);
                        } else {
                            sprintf_s(LogBuffer, LB_SIZE, "Useless block: ignoring");
                            DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
                        }
                    }
                    catch (incorrect_tag &e) {
                        sprintf_s(LogBuffer, LB_SIZE, "INCORRECT_TAG: [%s]", e.what());
                        DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
                    }
                }

            }

            if (buffer)
                delete[] buffer;
        }
        zip_fclose(file);
        sprintf_s(LogBuffer, LB_SIZE, "Read %d blocks: %lld blocks in Stash", NumBlocks, Blocks.size());
        DoLog(typeid(*this).name(), LogBuffer, LOG_INFO);
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
            sprintf_s(LogBuffer, LB_SIZE, "Logic Error writing block of type %s: %s", typeid(*Block).name(), ex.what());
            DoLog(typeid(*this).name(), LogBuffer, LOG_ERROR);
        }
    }

    return Buff;
}

void RMPage::AddBlock(RMBlock * Block) {
    Blocks.push_back(Block);

    if (Block->BlockType() == BT_SceneTree)
        IndexBlocks[((SceneTree*)Block)->node_id] = Block;
    else if (Block->BlockType() == BT_TreeNode)
        IndexBlocks[((TreeNode*)Block)->node_id] = Block;
    else if (dynamic_cast<RMSceneItemBlock*>(Block))
        IndexBlocks[((RMSceneItemBlock*)Block)->item_id] = Block;
    else if (Block->BlockType() == BT_RootText)
        IndexBlocks[((RootText*)Block)->texts[0].item_id] = Block;

}

void RMPage::DrawPage(void * DrawDetails) {
    DrawPageInit(DrawDetails);

    // We need to do two passes, so that we can be sure to have done the text anchors first
    for (auto const& [key, val] : IndexBlocks)
    {
        if (typeid(*val) == typeid(RootText))
        {
            sprintf_s(LogBuffer, LB_SIZE, "Drawing Text Block ID (%d, %d)", key.part1, key.part2);
            DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
            DrawTextItem(DrawDetails, (RootText*)val);
        }
    }

    for (auto const& [key, val] : IndexBlocks)
    {
        if (typeid(*val) == typeid(SceneLineItem))
        {
            sprintf_s(LogBuffer, LB_SIZE, "Drawing Line Block ID (%d, %d)", key.part1, key.part2);
            DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
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

