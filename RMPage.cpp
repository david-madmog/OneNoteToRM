#include "RMPage.h"

#pragma pack(1)
struct rm_frontmatter_header {
    char magic_text[32]; //         contents : reMarkable.lines file, version =
    char version_string; //        Version number, but encoded as a single UTF - 8 character.
    char ten_spaces[10]; //        contents : [0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20]
};

struct rm_BlockHead {
    UINT32  len_body; //            doc : Byte count for block's main body.
    unsigned char magic1;
    unsigned char MinVersion;
    unsigned char CurrentVersion; 
    unsigned char BlockType;
};


RMPage::RMPage()
{
    MaxX = 100;
    MaxY = 100;
    MinX = 0;
    MinY = 0;
}

void RMPage::Load(zip_file* file)
{
    zip_int64_t NumRead;
    char LogBuff[10240];

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
            
            buffer = (unsigned char *) malloc(BH.len_body);
            if (! buffer)
                NumRead = 0;

            // now the rest of the block
            if (NumRead > 0)
                NumRead = zip_fread(file, buffer, BH.len_body);
//            NumRead = zip_fread(file, buffer, 1024);

            if (NumRead > 0) {
                RMBlock* NewBlock = NULL;

                switch (BH.BlockType) {
                case 0:
                    NewBlock = new MigrationInfo();
                    break;
                case 1:
                    NewBlock = new SceneTree();
                    break;
                case 2:
                    NewBlock = new TreeNode();
                    break;
                case 3:
                    NewBlock = new SceneGlyphItem();
                    break;
                case 4:
                    NewBlock = new SceneGroupItem();
                    break;
                case 5:
                    NewBlock = new SceneLineItem();
                    break;
                case 6:
                    NewBlock = new SceneTextItem();
                    break;
                case 7:
                    NewBlock = new RootText();
                    break;
                case 8:
                    NewBlock = new SceneTombstoneItem();
                    break;
                case 9:
                    NewBlock = new AuthorIds();
                    break;
                case 10:
                    NewBlock = new PageInfo();
                    break;
                case 13:
                    NewBlock = new SceneInfo();
                    break;
                default:
                    sprintf_s(LogBuff, "**** Unknown Type %d len %d", BH.BlockType, BH.len_body);
                    break;
                }
//                DoLog(LogBuff);

                if (NewBlock) {
                    try {
                        NewBlock->ParseBuffer(buffer, BH.len_body, BH.CurrentVersion);

                        // Stash the block, and also build it into the tree
                        Blocks.push_back(NewBlock);
                        Tree.AddBlock(NewBlock);
                    }
                    catch (incorrect_tag &e) {
                        DoLog(LogBuff);
                        sprintf_s(LogBuff, "INCORRECT_TAG: [%s]", e.what());
                        DoLog(LogBuff);
                    }
                }

            }

            if (buffer)
                free(buffer);
        }
        zip_fclose(file);

    }

}
    
void RMPage::DrawPage(HDC hDC) {

    for (auto const& [key, val] : Tree.Tree)
    {
        for (auto const SIB : val.Children) {
            if (typeid(*SIB) == typeid(SceneLineItem))
                FindMinMax((SceneLineItem*)SIB);
        }
    }


    RECT rect;
    SetRect(&rect, 0, 0, MaxX - MinX, MaxY - MinY);
    FillRect(hDC, &rect, (HBRUSH)(COLOR_WINDOW+1));

    //for (RMSceneItemBlock &block: Tree)
    //{
    //    char LogBuff[10240];
    //    sprintf_s(LogBuff, "<D> %s ID (%d, %d)",
    //        typeid(block).name(), block.item_id.part1, block.item_id.part1);

    //    DoLog(LogBuff);
    //}

    char LogBuff[10240];
    //std::map<RM_CRDT_ID, RMTreeGroup> Tree;
    for (auto const& [key, val] : Tree.Tree)
    { 
        sprintf_s(LogBuff, "<D> ID (%d, %d)", val.node_ID.part1, val.node_ID.part1);
        DoLog(LogBuff);

//        std::vector<RMSceneItemBlock*> Children;

        for (auto const SIB : val.Children) {
            sprintf_s(LogBuff, "<D> Type:%s", typeid(*SIB).name());
            DoLog(LogBuff);

            if (typeid(*SIB) == typeid(SceneLineItem))
                DrawLineItem(hDC, (SceneLineItem*)SIB);
        }
    }

    sprintf_s(LogBuff, "<D> Range:(%d,%d) to (%d, %d)", MinX, MinY, MaxX, MaxY);
    DoLog(LogBuff);
}

void RMPage::DrawLineItem(HDC hDC, SceneLineItem* SLI)
{
    if (SLI->points.empty())
        return;

    HPEN hPen = CreatePen(PS_SOLID, SLI->points[0].width / 4, SLI->colour());
    HGDIOBJ hPenOld;

    hPenOld = SelectObject(hDC, hPen);

    MoveToEx(hDC, int(SLI->points[0].x) - MinX, int(SLI->points[0].y)-MinY,NULL);
    for (auto const& point : SLI->points)
    {
        LineTo(hDC, int(point.x) - MinX, int(point.y) - MinY);
    }

    SelectObject(hDC, hPenOld);
    DeleteObject(hPen);
}

void RMPage::FindMinMax(SceneLineItem* SLI)
{
    for (auto const& point : SLI->points)
    {
        MaxX = max(MaxX, int(point.x));
        MinX = min(MinX, int(point.x));
        MaxY = max(MaxY, int(point.y));
        MinY = min(MinY, int(point.y));
    }
}
