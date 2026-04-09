#include "framework.h"
#include "WindowRMPage.h"
#include "OneNoteToRM.h"
#include "ConversionConstants.h"

#define SHOW_LINE_ANCHORS 1

using namespace Gdiplus;

WindowRMPage::WindowRMPage()
    : RMPage()
{
}

WindowRMPage::WindowRMPage(std::string ID) 
    : RMPage(ID)
{
}

WindowRMPage::~WindowRMPage()
{
    for (auto& Block : Blocks)
        delete Block;
    Blocks.clear();
}


void WindowRMPage::DrawPageInit(void* DrawDetails)
{
    DrawDetailsParams* DD = (DrawDetailsParams*)DrawDetails;
    HDC hDC = DD->hDC;

//    for (auto const& [key, val] : IndexBlocks)
    for (auto & val : Blocks)
    {
        if (typeid(*val) == typeid(SceneInfo))
        {
            ((SceneInfo*)val)->GetPaperSize(paper_size[0], paper_size[1]);
            if (paper_size[0] < 100)
                paper_size[0] = PAGE_SIZE_X;
            if (paper_size[1] < 100)
                paper_size[1] = PAGE_SIZE_Y;
        }

        if (typeid(*val) == typeid(SceneLineItem))
            FindMinMax((SceneLineItem*)val);
    }

    std::wostringstream LB;
    LB << L"Paper Size " << paper_size[0] << L"x" << paper_size[1] << L", Lines in area (" 
        << LineExt.X << L"," << LineExt.Y << L") " << LineExt.Width << L"x" << LineExt.Height;
    DoLog(typeid(*this).name(), LB.str(), LOG_INFO);

//    RECT rect;
    SetRect(&DD->Rect, 0, 0, paper_size[0], paper_size[1]);
    FillRect(hDC,&DD->Rect, (HBRUSH)(COLOR_WINDOW + 1));

}

void WindowRMPage::DrawLineItem(void* DrawDetails, SceneLineItem* SLI)
{
    HDC hDC = ((DrawDetailsParams*)DrawDetails)->hDC;

    Point Origin((paper_size[0]/2) + LineExt.X, LINES_Y_START);
    Graphics graphics(hDC);

    if (SLI->points.empty())
        return;

    // find my anchor:
    RMBlock* Parent = IndexBlocks[SLI->parent_id]; // this should be a tree node, or could be a group item, in which case recurse
    if (Parent) {
        while (typeid(*Parent) == typeid(SceneGroupItem) || typeid(*Parent) == typeid(SceneTree)) {
            if (typeid(*Parent) == typeid(SceneGroupItem))
            {
                Parent = IndexBlocks[((SceneGroupItem*)Parent)->parent_id];
            }
            else
            {
                Parent = IndexBlocks[((SceneTree*)Parent)->parent_id];
            }

        }

        if (typeid(*Parent) == typeid(TreeNode)) {
            // So, get the anchor of the parent       
            RM_CRDT_ID Anchor = ((TreeNode*)Parent)->anchor_id.value;
            if (Anchor == RM_CRDT_ID{ 0, 0 } || Anchor == RM_CRDT_ID{ 0, -1 } || Anchor == RM_CRDT_ID{ 0, -2 })
            {
                Origin.X = (int)((TreeNode*)Parent)->anchor_origin_x.value - LineExt.X; // Null anchor - all good: draw from page origin
            }
            else if (Anchors.count(Anchor)) {
                // So, this line's origin is the finishing point of that item
                POINT AnchorNode = Anchors[Anchor];
                Origin = Point(AnchorNode.x, AnchorNode.y);
            }
            else {
                std::wostringstream LB;
                LB << L"Line Item's Parent's Anchor not found, Parent: " << ((TreeNode*)Parent)->node_id << L", Anchor: " << Anchor;
                DoLog(typeid(*this).name(), LB.str(), LOG_WARNING);
            }
        }
        else {
            std::wostringstream LB;
            LB << L"Line Item's Parent is not TreeNode: " << typeid(*Parent).name();
            DoLog(typeid(*this).name(), LB.str(), LOG_WARNING);
        }
    }
    else {
        std::wostringstream LB;
        LB << L"Line Item's Parent is NULL, Parent: " << SLI->parent_id;
        DoLog(typeid(*this).name(), LB.str(), LOG_WARNING);
    }

    Pen pen(SLI->colour(), (REAL)SLI->points[0].width / 4);
    pen.SetLineJoin(LineJoinRound);
    pen.SetEndCap(LineCapRound);

    if (SLI->tool_id == 21) // Calligraphy pen - special case
    {
        for (int i = 0; i < SLI->points.size() - 1; i++) {
            pen.SetWidth((REAL)(SLI->points[i].width / 4.0));
            graphics.DrawLine(&pen, (int)SLI->points[i].x + Origin.X, (int)SLI->points[i].y + Origin.Y,
                (int)SLI->points[i + 1].x + Origin.X, (int)SLI->points[i + 1].y + Origin.Y);

        }
    }
    else {
//        Point* Points = (Point*)malloc(SLI->points.size() * sizeof(Point));
        Point* Points = new Point[SLI->points.size()];
        for (int i = 0; i < SLI->points.size(); i++)
#pragma warning ( push )
#pragma warning( disable : 6386)
            Points[i] = Point((int)SLI->points[i].x + Origin.X, (int)SLI->points[i].y + Origin.Y);
#pragma warning ( pop )

        graphics.DrawLines(&pen, Points, (int)SLI->points.size());
        delete[] Points;
    }

#if SHOW_LINE_ANCHORS
    Pen GPen(Color(0, 255, 0), 1);
    graphics.DrawLine(&GPen, (int)SLI->points[0].x + Origin.X, (int)SLI->points[0].y + Origin.Y, (int)Origin.X, (int)Origin.Y);

    std::wostringstream NI;
    NI << ((TreeNode*)Parent)->node_id << L" via " << SLI->parent_id;
    FontFamily fontFamily(L"Arial"); 
    Font *font = new Font(&fontFamily, 10, FontStyleRegular, UnitPixel);
    SolidBrush  solidBrush(Color(255, 0, 127, 0));
    StringFormat form(StringFormat::GenericTypographic()); // this is needed to prevent characters from including padding
    graphics.DrawString(NI.str().c_str(), (int)NI.str().size(), font,
        PointF(SLI->points[0].x + Origin.X, SLI->points[0].y + Origin.Y), &form, &solidBrush);
#endif
}

Font* WindowRMPage::GetRMFont(int format_code) {
    //# Based on a rm file having 4 anchors based on the line height I was able to find a value of
    //    # 69.5, but decided on 70 (to keep integer values)
    //    si.ParagraphStyle.PLAIN: 70,
    //    si.ParagraphStyle.BULLET : 35,
    //    si.ParagraphStyle.BULLET2 : 35,
    //    si.ParagraphStyle.BOLD : 70,
    //    si.ParagraphStyle.HEADING : 150,
    //    si.ParagraphStyle.CHECKBOX : 35,
    //    si.ParagraphStyle.CHECKBOX_CHECKED : 35,

    //BASIC = 0
    //    PLAIN = 1
    //    HEADING = 2
    //    BOLD = 3
    //    BULLET = 4
    //    BULLET2 = 5
    //    CHECKBOX = 6
    //    CHECKBOX_CHECKED = 7
    Font* font;

    switch (format_code)
    {
    case 1:
    {
        FontFamily fontFamily(L"Arial");
        font = new Font(&fontFamily, 32, FontStyleRegular, UnitPixel);
        break;
    }
    case 2:
    {
        FontFamily fontFamily(L"Book Antiqua");
        font = new Font(&fontFamily, 64, FontStyleRegular, UnitPixel);
        break;
    }
    case 3:
    {
        FontFamily fontFamily(L"Arial");
        font = new Font(&fontFamily, 32, FontStyleBold, UnitPixel);
        break;
    }
    case 0:
    case 4:
    case 5:
    case 6:
    case 7:
    default:
    {
        FontFamily fontFamily(L"Arial");
        font = new Font(&fontFamily, 16, FontStyleRegular, UnitPixel);
    }
    }

    return font;
}

void WindowRMPage::DrawTextItem(void* DrawDetails, RootText* RT)
{
    struct TextRun
    {
        std::wstring Text;
        int format_code;
        RM_CRDT_ID AnchorID;
    };
    std::vector<TextRun> TextRuns;

    HDC hDC = ((DrawDetailsParams*)DrawDetails)->hDC;

    Graphics graphics(hDC);
    //    PointF DrawPos(TEXT_X_START, TEXT_Y_START);
    PointF DrawPos((REAL)RT->pos_x + RM_X_OFFSET, (REAL)RT->pos_y);
    RectF BoundingBox;
    //WCHAR WC;

    Font* font = NULL;
    int format_code = 0;

    for (auto& format : RT->formats)
    {
        std::wostringstream LB;
        LB << L"FONT: Formats are at " << format.charID << L" :" << format.format_code;
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
        if (format.charID == RM_CRDT_ID{ 0, 0 }) {
            font = GetRMFont(format.format_code);
            format_code = format.format_code;
        }
    }

    for (auto& text : RT->texts) {
       TextRun TR;
        TR.format_code = format_code;
        TR.Text = L"";
        if (text.value)
        {
            char* Message = text.value;
            TR.AnchorID = text.item_id;
            RM_CRDT_ID CharAnchorID = text.item_id;

            // We need to go through one char at a time to apply formats
            while (*Message) {
                TR.Text.append(1, (const wchar_t)*Message);
                // now see if we need to change format for the current Run
                for (auto& format : RT->formats)
                {
                    if (format.charID == CharAnchorID) {
                        TR.format_code = format.format_code;
                        format_code = format.format_code;
                        std::wostringstream LB;
                        LB << L"FONT: changing to code " << format.format_code;
                        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
                        break;
                    }
                    if (*Message == '\n') {
                        // New line starts a new run
                        TextRuns.push_back(TR);
                        TR.Text = L"";
                    }
                }
                CharAnchorID.part2++;
                Message++;
            }
        }
        TextRuns.push_back(TR);
    }

    // Now we can draw the text run - one char at a time to record the co-ords for anchor points
    SolidBrush  solidBrush(Color(255, 0, 0, 0));
    StringFormat form(StringFormat::GenericTypographic()); // this is needed to prevent characters from including padding
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    for (auto& TextRun : TextRuns)
    {
        RM_CRDT_ID AnchorID = TextRun.AnchorID;
        font = GetRMFont(TextRun.format_code);
        std::wostringstream LB;
        LB << L"Text Run code " << TextRun.format_code << L" for text " << TextRun.Text;
        DoLog(typeid(*this).name(), LB.str(), LOG_INFO);


        for (wchar_t& TC : TextRun.Text) {

            graphics.DrawString(&TC, 1, font, DrawPos, &form, &solidBrush);
            if (TC == ' ') // measure string won't measure a space, so we have to expand it
                TC = 'X';
            graphics.MeasureString(&TC, 1, font, DrawPos, &form, &BoundingBox);
            //                            Pen GPen(Color(0,255,0), 1);
            //                            graphics.DrawRectangle(&GPen, BoundingBox);

            if (TC == '\n') {
                DrawPos.Y += BoundingBox.Height;
                DrawPos.X = (REAL)RT->pos_x + RM_X_OFFSET;
            }
            else {
                DrawPos.X += BoundingBox.Width;
            }

            POINT BR = { (int)DrawPos.X, (int)DrawPos.Y };
            Anchors[AnchorID] = BR;
            AnchorID.part2++;

        }
    }
/*
    for (auto& text : RT->texts) {
        if (text.value)
        {
            char* Message = text.value;

            // We need to go through one char at a time to calculate the anchor points and apply formats
            while (*Message) {

                WC = (WCHAR)*Message;

                if (font)
                {
                    std::wostringstream LB;
                    LB << L"FONT: code: " << *Message << L" style: " << font->GetStyle() << L" size: " << font->GetSize() << L" - Anchor " << AnchorID;
                    DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
                }


                graphics.DrawString(&WC, 1, font, DrawPos, &form, &solidBrush);
                if (*Message == ' ') // measure string won't measure a space, so we have to expand it
                    WC = 'X';
                graphics.MeasureString(&WC, 1, font, DrawPos, &form, &BoundingBox);
                //                            Pen GPen(Color(0,255,0), 1);
                //                            graphics.DrawRectangle(&GPen, BoundingBox);

                if (*Message == '\n') {
                    DrawPos.Y += BoundingBox.Height;
                    DrawPos.X = (REAL)RT->pos_x + RM_X_OFFSET;

                    // So, look ahead, and see if there's another format def before the next newline
                    RM_CRDT_ID lookaheadID = AnchorID;
                    char* lookahead = Message + 1;
                    for (; *lookahead != '\n' && *lookahead != '\0' ; lookahead++)
                    {                      
                        for (auto& format : RT->formats)
                        {
                            if (format.charID == lookaheadID) {
                                font = GetRMFont(format.format_code);
                                std::wostringstream LB;
                                LB << L"FONT: changing to code " << format.format_code << L" at ID " << lookaheadID;
                                DoLog(typeid(*this).name(), LB.str(), LOG_INFO);
                                goto LOOP_EXIT;
                            }
                        }
                    }
                LOOP_EXIT: 
                    ;
                }
                else {
                    DrawPos.X += BoundingBox.Width;
                }

                // now see if we need to change format for the next char
                for (auto& format : RT->formats)
                {
                    if (format.charID == AnchorID) {
                        font = GetRMFont(format.format_code);
                        std::wostringstream LB;
                        LB << L"FONT: changing to code " << format.format_code;
                        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
                        break;
                    }
                }


                POINT BR = { (int)DrawPos.X, (int)DrawPos.Y };
                Anchors[AnchorID] = BR;
                AnchorID.part2++;

                Message++;
            }
        }
    }
    */
}

void WindowRMPage::FindMinMax(SceneLineItem* SLI)
{
    for (auto const& point : SLI->points)
    {
        LineExt.X = min(LineExt.X, int(point.x));
        LineExt.Width = max(LineExt.Width, int(point.x) - LineExt.X);
        LineExt.Y = min(LineExt.Y, int(point.y));
        LineExt.Height = max(LineExt.Height, int(point.y) - LineExt.Y);
    }
}
