#include "framework.h"
#include "ToOneRMPage.h"
#include "WindowRMPage.h"
#include "GraphDoc.h"

ToOneRMPage::ToOneRMPage() 
	: RMPage()
{
}

ToOneRMPage::ToOneRMPage(std::string id)
	: RMPage(id)
{
}

ToOneRMPage::~ToOneRMPage()
{
    for (auto& Block : Blocks)
        delete Block;
    Blocks.clear();
}



void ToOneRMPage::DrawPageInit(void* DrawDetails) {

    for (auto const& [key, val] : IndexBlocks)
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

    // Create our destination page
//    OnePage = new WindowONEPage;
    ((GraphDoc<WindowONEPage>*)DrawDetails)->AddPage(&OnePage);
    std::wostringstream T;
    T << PageTitle.c_str();
    OnePage.PageTitle = T.str();
}

void ToOneRMPage::FindMinMax(SceneLineItem* SLI)
{
    for (auto const& point : SLI->points)
    {
        LineExt.X = min(LineExt.X, int(point.x));
        LineExt.Width = max(LineExt.Width, int(point.x) - LineExt.X);
        LineExt.Y = min(LineExt.Y, int(point.y));
        LineExt.Height = max(LineExt.Height, int(point.y) - LineExt.Y);
    }
}


void ToOneRMPage::DrawLineItem(void* DrawDetails, SceneLineItem* SLI)
{

    INK_Point Origin((paper_size[0] / 2) + LineExt.X, LINES_Y_START, 100);

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
                Origin = INK_Point(AnchorNode.x, AnchorNode.y, 100);
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

    INK_Trace * Trace = new INK_Trace();
    Trace->colour = SLI->colour();
    Trace->thickness_scale = (DOUBLE)SLI->points[0].width * RM_TO_ONE_THICK_FACTOR ;
//    Trace->thickness_scale = SLI->thickness_scale * RM_TO_ONE_THICK_FACTOR ;
    std::wostringstream tid;
    tid << L"{" << SLI->tool_id << SLI->colour().GetValue() << L"}";
    Trace->tool_id = tid.str() ;
    tid << SLI->item_id;
    Trace->trace_id = tid.str();
    for (int i = 0; i < SLI->points.size() - 1; i++) {
        INK_Point P;
        P.X = (int)(SLI->points[i].x + Origin.X) * RM_TO_ONE_XY_SCALE_FACTOR;
        P.Y = (int)(SLI->points[i].y + Origin.Y) * RM_TO_ONE_XY_SCALE_FACTOR;
        P.F = (int)SLI->points[i].width * RM_TO_ONE_LINE_FACTOR;
//        P.F = (int)SLI->points[i].pressure * RM_TO_ONE_PRESSURE_FACTOR;
        Trace->points.push_back(P);
    }

    OnePage.InkTraces.push_back(Trace);

#if FALSE
    Trace = new INK_Trace();
    Trace->colour = Gdiplus::Color::Green;
    Trace->thickness_scale = 10;

    tid << L"{123432}";
    Trace->tool_id = tid.str();
    tid << SLI->item_id << L"-B";
    Trace->trace_id = tid.str();
    INK_Point P;
    P.X = (int)(SLI->points[0].x + Origin.X) * RM_TO_ONE_SCALE_FACTOR;
    P.Y = (int)(SLI->points[0].y + Origin.Y) * RM_TO_ONE_SCALE_FACTOR;
    P.F = (int)SLI->points[0].width * RM_TO_ONE_SCALE_FACTOR;
    Trace->points.push_back(P);
    P.X = (int)(Origin.X) * RM_TO_ONE_SCALE_FACTOR;
    P.Y = (int)(Origin.Y) * RM_TO_ONE_SCALE_FACTOR;
    P.F = (int)SLI->points[0].width * RM_TO_ONE_SCALE_FACTOR;
    Trace->points.push_back(P);
    OnePage.InkTraces.push_back(Trace);
#endif
}

std::wstring ToOneRMPage::GetRMFont(int format_code) {
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
    if (format_code == 2)
        return L"Book Antiqua";
    else if (format_code == 3)
        return L"Arial Bold";
    else
        return L"Arial";
}

int ToOneRMPage::GetRMFontSize(int format_code) {

    switch (format_code)
    {
    case 1:
        return 32;
        break;
    case 2:
        return 64;
        break;
    case 4:
        return 32;
    case 0:
    case 3:
    case 5:
    case 6:
    case 7:
    default:
        return 16;
    }
    return 16;
}



void ToOneRMPage::DrawTextItem(void* DrawDetails, RootText* RT)
{
    POINT DrawPos((LONG)RT->pos_x + RM_X_OFFSET, (LONG)RT->pos_y);
    WCHAR WC;

    ONE_Text * Text = new ONE_Text;
    ONE_TextSpan rootSpan;

 //   Font* font = NULL;

    for (auto& format : RT->formats)
    {
        std::wostringstream LB;
        LB << L"FONT: Formats are at " << format.charID << L" :" << format.format_code;
        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
        if (format.charID == RM_CRDT_ID{ 0, 0 }) {
            rootSpan.Font = GetRMFont(format.format_code);
            rootSpan.FontSize = GetRMFontSize(format.format_code);
        }
    }

    Text->Left = DrawPos.x;
    Text->Top = DrawPos.y;

    for (auto& text : RT->texts) {
        if (text.value)
        {
            ONE_TextSpan span = ONE_TextSpan();
            span.Text = L"";

            span.Font = rootSpan.Font;
            span.FontSize = rootSpan.FontSize;

            std::wostringstream buff;
            buff << text.value;
            span.Text.append(buff.str());
            Text->Texts.push_back(span);
             
            char* Message = text.value;
            RM_CRDT_ID AnchorID = text.item_id;

            // We need to go through one char at a time to calculate the anchor points and apply formats
            while (*Message) {

                WC = (WCHAR)*Message;

                //if (*Message == ' ') // measure string won't measure a space, so we have to expand it
                //    WC = 'X';
                //graphics.MeasureString(&WC, 1, font, DrawPos, &form, &BoundingBox);
                ////                            Pen GPen(Color(0,255,0), 1);
                ////                            graphics.DrawRectangle(&GPen, BoundingBox);

                if (*Message == '\n') {
                    DrawPos.y += rootSpan.FontSize;  // Horrible approximation, as we don't really have a reference to calculate otherwise
                    DrawPos.x = (LONG)RT->pos_x + RM_X_OFFSET;
                }
                else {
                    DrawPos.x += rootSpan.FontSize;
                }

                // now see if we beed to change format for the next char
                for (auto& format : RT->formats)
                {
                    if (format.charID == AnchorID) {
                        rootSpan.Font = GetRMFont(format.format_code);
                        rootSpan.FontSize = GetRMFontSize(format.format_code);
                        std::wostringstream LB;
                        LB << L"FONT: changing to code " << format.format_code;
                        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
                        break;
                    }
                }

                POINT BR = { (int)DrawPos.x, (int)DrawPos.y };
                Anchors[AnchorID] = BR;
                AnchorID.part2++;

                Message++;
            }
        }
    }

    OnePage.TextDivs.push_back(Text);
}

