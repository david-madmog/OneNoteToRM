#include "WindowRMPage.h"

using namespace Gdiplus;

WindowRMPage::WindowRMPage()
    : RMPage()
{
    MaxX = 100;
    MaxY = 100;
    MinX = 0;
    MinY = 0;
}

WindowRMPage::WindowRMPage(std::string ID) 
    : RMPage(ID)
{
    MaxX = 100;
    MaxY = 100;
    MinX = 0;
    MinY = 0;
}

void WindowRMPage::DrawPageInit(void* DrawDetails)
{
    HDC hDC = (HDC)DrawDetails;

    for (auto const& [key, val] : IndexBlocks)
    {
        if (typeid(*val) == typeid(SceneLineItem))
            FindMinMax((SceneLineItem*)val);
    }

    RECT rect;
    SetRect(&rect, 0, 0, MaxX - MinX, MaxY - MinY);
    FillRect(hDC, &rect, (HBRUSH)(COLOR_WINDOW + 1));
}

void WindowRMPage::DrawLineItem(void* DrawDetails, SceneLineItem* SLI)
{
    HDC hDC = (HDC)DrawDetails;

    Point Origin(-MinX, -MinY);
    Graphics graphics(hDC);

    if (SLI->points.empty())
        return;

    // find my anchor:
    RMBlock* Parent = IndexBlocks[SLI->parent_id]; // this should be a tree node, or could be a group item, in which case recurse
    if (Parent) {
        while (typeid(*Parent) == typeid(SceneGroupItem)) {
            Parent = IndexBlocks[((SceneGroupItem*)Parent)->parent_id];
        }

        if (typeid(*Parent) == typeid(TreeNode)) {
            // So, get the anchor of the parent       
            RM_CRDT_ID Anchor = ((TreeNode*)Parent)->anchor_id.value;
            if (Anchor == RM_CRDT_ID{ 0, 0 } || Anchor == RM_CRDT_ID{ 0, -1 } || Anchor == RM_CRDT_ID{ 0, -2 })
            {
                ; // Null anchor - all good: draw from page origin
            }
            else if (Anchors.count(Anchor)) {
                // So, this line's origin is the finishing point of that item
                POINT AnchorNode = Anchors[Anchor];
                Origin = Point(AnchorNode.x, AnchorNode.y);
            }
            else {
                sprintf_s(LogBuffer, LB_SIZE, "Line Item's Parent's Anchor not found, Parent: (%d, %d), Anchor: (%d, %d)",
                    ((TreeNode*)Parent)->node_id.part1, ((TreeNode*)Parent)->node_id.part2, Anchor.part1, Anchor.part2
                );
                DoLog(typeid(*this).name(), LogBuffer, LOG_WARNING);
            }
        }
        else {
            sprintf_s(LogBuffer, LB_SIZE, "Line Item's Parent is Not TreeNode: %s", typeid(*Parent).name());
            DoLog(typeid(*this).name(), LogBuffer, LOG_WARNING);
        }
    }
    else {
        sprintf_s(LogBuffer, LB_SIZE, "Line Item's Parent's is NULL, Parent: (%d, %d)",
            SLI->parent_id.part1, SLI->parent_id.part2
        );
        DoLog(typeid(*this).name(), LogBuffer, LOG_WARNING);

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
        Point* Points = (Point*)malloc(SLI->points.size() * sizeof(Point));
        if (Points != NULL)
            for (int i = 0; i < SLI->points.size(); i++)
                Points[i] = Point((int)SLI->points[i].x + Origin.X, (int)SLI->points[i].y + Origin.Y);

        graphics.DrawLines(&pen, Points, (int)SLI->points.size());
        free(Points);

        //Pen GPen(Color(0, 255, 0), 1);
        //graphics.DrawLine(&GPen, (int)SLI->points[0].x + Origin.X, (int)SLI->points[0].y + Origin.Y,
        //    (int)Origin.X, (int)Origin.Y);

    }
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
constexpr auto TEXT_X_START = 50;
constexpr auto TEXT_Y_START = 150;
    HDC hDC = (HDC)DrawDetails;

    Graphics graphics(hDC);
    PointF DrawPos(TEXT_X_START, TEXT_Y_START);
    RectF BoundingBox;
    WCHAR WC;

    Font* font = NULL;

    for (auto& format : RT->formats)
    {
        sprintf_s(LogBuffer, LB_SIZE, "FONT: Formats are at (%d, %d): %d",
            format.charID.part1, format.charID.part2, format.format_code
        );
        DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
        if (format.charID == RM_CRDT_ID{ 0, 0 }) {
            font = GetRMFont(format.format_code);
        }
    }

    SolidBrush  solidBrush(Color(255, 0, 0, 0));
    StringFormat form(StringFormat::GenericTypographic()); // this is needed to prevent characters from including paddinf
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    for (auto& text : RT->texts) {
        if (text.value)
        {
            char* Message = text.value;
            RM_CRDT_ID AnchorID = text.item_id;

            // We need to go through one char at a time to calculate the anchor points and apply formats
            while (*Message) {

                WC = (WCHAR)*Message;

                if (font)
                {
                    sprintf_s(LogBuffer, LB_SIZE, "FONT: code: %c style: %d size %f - Anchor (%d, %d)",
                        *Message, font->GetStyle(), font->GetSize(), AnchorID.part1, AnchorID.part2
                    );
                    DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
                }


                graphics.DrawString(&WC, 1, font, DrawPos, &form, &solidBrush);
                if (*Message == ' ') // measure string won't measure a space, so we have to expand it
                    WC = 'X';
                graphics.MeasureString(&WC, 1, font, DrawPos, &form, &BoundingBox);
                //                            Pen GPen(Color(0,255,0), 1);
                //                            graphics.DrawRectangle(&GPen, BoundingBox);

                if (*Message == '\n') {
                    DrawPos.Y += BoundingBox.Height;
                    DrawPos.X = TEXT_X_START;
                }
                else {
                    DrawPos.X += BoundingBox.Width;
                }

                // now see if we beed to change format for the next char
                for (auto& format : RT->formats)
                {
                    if (format.charID == AnchorID) {
                        font = GetRMFont(format.format_code);
                        sprintf_s(LogBuffer, LB_SIZE, "FONT: changing to code %d", format.format_code);
                        DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
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
}


void WindowRMPage::FindMinMax(SceneLineItem* SLI)
{
    for (auto const& point : SLI->points)
    {
        MaxX = max(MaxX, int(point.x));
        MinX = min(MinX, int(point.x));
        MaxY = max(MaxY, int(point.y));
        MinY = min(MinY, int(point.y));
    }
}
