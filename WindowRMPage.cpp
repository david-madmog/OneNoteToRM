#include "pch.h"

#include "WindowRMPage.h"
#include "OneNoteToRM.h"
#include "ConversionConstants.h"

#define SHOW_LINE_ANCHORS 0

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

    SetRect(&DD->Rect, 0, 0, paper_size[0], paper_size[1]);
//    SetRect(&DD->Rect, 0, 0, LineExt.Width, LineExt.Height);
    FillRect(hDC,&DD->Rect, (HBRUSH)(COLOR_WINDOW + 1));

}

void WindowRMPage::DrawLineItem(void* DrawDetails, SceneLineItem* SLI)
{
    HDC hDC = ((DrawDetailsParams*)DrawDetails)->hDC;

//    Point Origin((paper_size[0] / 2) + LineExt.X, LINES_Y_START);
    Point Origin(-LineExt.X, -LineExt.Y);
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
//                Origin.X = (int)((TreeNode*)Parent)->anchor_origin_x.value + RM_X_OFFSET; // Null anchor - all good: draw from page origin
                Origin.Y = LINES_Y_START - LineExt.Y;
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
            graphics.DrawLine(&pen, (int)(SLI->points[i].x * X_SCALE) + Origin.X, (int)(SLI->points[i].y * Y_SCALE) + Origin.Y,
                (int)(SLI->points[i + 1].x * X_SCALE) + Origin.X, (int)(SLI->points[i + 1].y * Y_SCALE) + Origin.Y);

        }
    }
    else {
//        Point* Points = (Point*)malloc(SLI->points.size() * sizeof(Point));
        Point* Points = new Point[SLI->points.size()];
        for (int i = 0; i < SLI->points.size(); i++)
#pragma warning ( push )
#pragma warning( disable : 6386)
            Points[i] = Point((int)(SLI->points[i].x * X_SCALE) + Origin.X, (int)(SLI->points[i].y * Y_SCALE) + Origin.Y);
#pragma warning ( pop )

        graphics.DrawLines(&pen, Points, (int)SLI->points.size());
        delete[] Points;
    }

#if SHOW_LINE_ANCHORS
    Pen GPen(Color(0, 255, 0), 1);
    graphics.DrawLine(&GPen, (int)(SLI->points[0].x * X_SCALE) + Origin.X, (int)(SLI->points[0].y * Y_SCALE) + Origin.Y, (int)Origin.X, (int)Origin.Y);

    if (Parent)
    {
        std::wostringstream NI;
        NI << ((TreeNode*)Parent)->node_id << L" via " << SLI->parent_id;
        FontFamily fontFamily(L"Arial");
        Font* font = new Font(&fontFamily, 10, FontStyleRegular, UnitPixel);
        SolidBrush  solidBrush(Color(255, 0, 127, 0));
        StringFormat form(StringFormat::GenericTypographic()); // this is needed to prevent characters from including padding
        graphics.DrawString(NI.str().c_str(), (int)NI.str().size(), font,
            PointF((REAL)(SLI->points[0].x * X_SCALE) + Origin.X, (REAL)(SLI->points[0].y * Y_SCALE) + Origin.Y), &form, &solidBrush);
    }
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
    Font* font;

    switch (format_code)
    {
    case 1: // PLAIN = 1
    {
        FontFamily fontFamily(L"Arial");
        font = new Font(&fontFamily, 24, FontStyleRegular, UnitPixel);
        break;
    }
    case 2: //    HEADING = 2
    {
        FontFamily fontFamily(L"Book Antiqua");
        font = new Font(&fontFamily, 48, FontStyleRegular, UnitPixel);
        break;
    }
    case 3:  //    BOLD = 3
    {
        FontFamily fontFamily(L"Arial");
        font = new Font(&fontFamily, 24, FontStyleBold, UnitPixel);
        break;
    }
    case 0: //BASIC = 0
    case 4://    BULLET = 4
    case 5://    BULLET2 = 5
    case 6://    CHECKBOX = 6
    case 7://    CHECKBOX_CHECKED = 7
    default:
    {
        FontFamily fontFamily(L"Arial");
        font = new Font(&fontFamily, 24, FontStyleRegular, UnitPixel);
    }
    }

    return font;
}

void WindowRMPage::DrawTextItem(void* DrawDetails, RootText* RT)
{
    struct TextRun
    {
        std::wstring Text;
        int format_code = 0;
        RM_CRDT_ID AnchorID;
    };
    std::vector<TextRun> TextRuns;

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
                if (*Message == '\n') {
                    // New line starts a new run
                    TextRuns.push_back(TR);
                    TR.Text = L"";
                    TR.AnchorID = CharAnchorID;
                    // Subsequent root text chunbk runs will have code 0 unless/until overridden by an explicit code
                    TR.format_code = 0;
                }
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
                }
                CharAnchorID.part2++;
                Message++;
            }
        }
        if (TR.Text.length() > 0)
            TextRuns.push_back(TR);
    }

    // Second pass to add in bullets
    for (auto& TextRun : TextRuns)
    {
        switch (TextRun.format_code) {
        case 4:
        case 5:
            TextRun.Text =  L"• " + TextRun.Text;   // Bullet
            break;
        case 6:
            TextRun.Text = (wchar_t)0x25A1 + L" " + TextRun.Text; //9633
            break;
        case 7:
            TextRun.Text = (wchar_t)0x221A + L" " + TextRun.Text; // 8730
            break;
        }
    }

    HDC hDC = ((DrawDetailsParams*)DrawDetails)->hDC;

    Graphics graphics(hDC);
    //    PointF DrawPos(TEXT_X_START, TEXT_Y_START);
    PointF DrawPos((REAL)(RT->pos_x + RM_X_OFFSET), (REAL)RT->pos_y + TEXT_Y_START);
    RectF BoundingBox;
    //WCHAR WC;



    // Now we can draw the text run - one char at a time to record the co-ords for anchor points
    SolidBrush  solidBrush(Color(255, 0, 0, 0));
    StringFormat form(StringFormat::GenericTypographic()); // this is needed to prevent characters from including padding
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    for (auto& TextRun : TextRuns)
    {
        RM_CRDT_ID AnchorID = TextRun.AnchorID;
        font = GetRMFont(TextRun.format_code);
        std::wostringstream LB;
        std::wstring TRT(TextRun.Text);
        std::replace(TRT.begin(), TRT.end(), L'\n', L'¶');
        LB << L"Text Run code " << TextRun.format_code << L" for text " << TRT << L" Starting at (" << DrawPos.X << L", " << DrawPos.Y << L") Anchor " << AnchorID;
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);


        for (wchar_t& TC : TextRun.Text) {
#if SHOW_LINE_ANCHORS
            if (TC == '\n')
                graphics.DrawString(L"¶", 1, font, DrawPos, &form, &solidBrush);
#endif
            graphics.DrawString(&TC, 1, font, DrawPos, &form, &solidBrush);
            if (TC == ' ') // measure string won't measure a space, so we have to expand it
                TC = 'X';
            graphics.MeasureString(&TC, 1, font, DrawPos, &form, &BoundingBox);

#if SHOW_LINE_ANCHORS
            Pen GPen(Color(0,255,0), 1);
            graphics.DrawRectangle(&GPen, BoundingBox);
#endif

            if (TC == '\n') {
                DrawPos.Y += BoundingBox.Height;
                DrawPos.X = (REAL)(RT->pos_x + RM_X_OFFSET);
            }
            else {
                DrawPos.X += BoundingBox.Width;
            }

            POINT BR = { (int)DrawPos.X, (int)(DrawPos.Y + BoundingBox.Height) };
            Anchors[AnchorID] = BR;
            AnchorID.part2++;

        }
    }
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
