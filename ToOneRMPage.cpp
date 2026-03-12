#include "ToOneRMPage.h"
#include "WindowRMPage.h"

#define RM_TO_ONE_SCALE_FACTOR 20


ToOneRMPage::ToOneRMPage() 
	: RMPage()
{
    MaxX = 100;
    MaxY = 100;
    MinX = 0;
    MinY = 0;
}

ToOneRMPage::ToOneRMPage(std::string id)
	: RMPage(id)
{
    MaxX = 100;
    MaxY = 100;
    MinX = 0;
    MinY = 0;
}

void ToOneRMPage::DrawPageInit(void* DrawDetails) {

    for (auto const& [key, val] : IndexBlocks)
    {
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
        MaxX = max(MaxX, int(point.x));
        MinX = min(MinX, int(point.x));
        MaxY = max(MaxY, int(point.y));
        MinY = min(MinY, int(point.y));
    }
}


void ToOneRMPage::DrawLineItem(void* DrawDetails, SceneLineItem* SLI)
{

    INK_Point Origin(-MinX, -MinY, 100);

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
                Origin = INK_Point(AnchorNode.x, AnchorNode.y, 100);
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

    INK_Trace * Trace = new INK_Trace();
    Trace->colour = SLI->colour();
    Trace->thickness_scale = (DOUBLE)SLI->points[0].width * RM_TO_ONE_SCALE_FACTOR / 2 ;

    std::wostringstream tid;
    tid << L"{" << SLI->tool_id << SLI->colour().GetValue() << L"}";
    Trace->tool_id = tid.str() ;
    tid << SLI->item_id;
    Trace->trace_id = tid.str();
    for (int i = 0; i < SLI->points.size() - 1; i++) {
        INK_Point P;
        P.X = (int)(SLI->points[i].x + Origin.X) * RM_TO_ONE_SCALE_FACTOR;
        P.Y = (int)(SLI->points[i].y + Origin.Y) * RM_TO_ONE_SCALE_FACTOR;
        P.F = (int)SLI->points[i].width * RM_TO_ONE_SCALE_FACTOR ;
        Trace->points.push_back(P);
    }

    OnePage.InkTraces.push_back(Trace);

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
    case 3:
        return 32;
    case 0:
    case 4:
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
    constexpr auto TEXT_X_START = 50;
    constexpr auto TEXT_Y_START = 150;

    POINT DrawPos(TEXT_X_START, TEXT_Y_START);
    WCHAR WC;

    ONE_Text * Text = new ONE_Text;
    ONE_TextSpan rootSpan;

 //   Font* font = NULL;

    for (auto& format : RT->formats)
    {
        sprintf_s(LogBuffer, LB_SIZE, "FONT: Formats are at (%d, %d): %d",
            format.charID.part1, format.charID.part2, format.format_code
        );
        DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
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
                    DrawPos.x = TEXT_X_START;
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
                        sprintf_s(LogBuffer, LB_SIZE, "FONT: changing to code %d", format.format_code);
                        DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
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

