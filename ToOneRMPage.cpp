#include "framework.h"
#include "ToOneRMPage.h"
#include "WindowRMPage.h"
#include "GraphDoc.h"

#define SHOW_LINE_ANCHORS 0


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

    for (auto& val : Blocks)
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

//    INK_Point Origin((paper_size[0] / 2) + LineExt.X, LINES_Y_START, 100);
    INK_Point Origin(-LineExt.X, -LineExt.Y, 100);

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

#if SHOW_LINE_ANCHORS
    Trace = new INK_Trace();
    Trace->colour = Gdiplus::Color::Green;
    Trace->thickness_scale = 10;

    tid << L"{123432}";
    Trace->tool_id = tid.str();
    tid << SLI->item_id << L"-B";
    Trace->trace_id = tid.str();
    INK_Point P;
    P.X = (int)(SLI->points[0].x + Origin.X) * RM_TO_ONE_XY_SCALE_FACTOR;
    P.Y = (int)(SLI->points[0].y + Origin.Y) * RM_TO_ONE_XY_SCALE_FACTOR;
    P.F = (int)SLI->points[0].width * RM_TO_ONE_LINE_FACTOR;
    Trace->points.push_back(P);
    P.X = (int)(Origin.X) * RM_TO_ONE_XY_SCALE_FACTOR;
    P.Y = (int)(Origin.Y) * RM_TO_ONE_XY_SCALE_FACTOR;
    P.F = (int)SLI->points[0].width * RM_TO_ONE_LINE_FACTOR;
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
    if (format_code == 2)
        return 48;
    else
        return 32;
}



void ToOneRMPage::DrawTextItem(void* DrawDetails, RootText* RT)
{
    POINT DrawPos((LONG)RT->pos_x + RM_X_OFFSET, (LONG)RT->pos_y + TEXT_Y_START);
//    WCHAR WC;

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
        ONE_TextSpan span = ONE_TextSpan();
        span.Text = L"";

        if (text.value)
        {
            RM_CRDT_ID AnchorID = text.item_id;

            span.Font = rootSpan.Font;
            span.FontSize = rootSpan.FontSize;

            char* Message = text.value;
            // We need to go through one char at a time to apply formats
            while (*Message) {
                span.Text.append(1, (const wchar_t)*Message);
                if (*Message == '\n') {
                    // New line starts a new run
                    Text->Texts.push_back(span);
                    span.Text = L"";
                    //TR.AnchorID = CharAnchorID;
                    // Subsequent root text chunk runs will have code 0 unless/until overridden by an explicit code
                    span.Font = GetRMFont(0);
                    span.FontSize = GetRMFontSize(0);
                    DrawPos.y += rootSpan.FontSize;  // Horrible approximation, as we don't really have a reference to calculate otherwise
                    DrawPos.x = (LONG)RT->pos_x + RM_X_OFFSET;
                }
                else {
                    DrawPos.x += rootSpan.FontSize;
                }

                // now see if we need to change format for the current Run
                for (auto& format : RT->formats)
                {
                    if (format.charID == AnchorID) {
                        span.Font = GetRMFont(format.format_code);
                        span.FontSize = GetRMFontSize(format.format_code);
                        //        format_code = format.format_code;
                        std::wostringstream LB;
                        LB << L"FONT: changing to code " << format.format_code;
                        DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
                        break;
                    }
                }

                POINT BR = {(int)DrawPos.x, (int)DrawPos.y };
                Anchors[AnchorID] = BR;
                AnchorID.part2++;
                Message++;
            }
        }
        if (span.Text.length() > 0)
            Text->Texts.push_back(span);
    }

    OnePage.TextDivs.push_back(Text);
}

