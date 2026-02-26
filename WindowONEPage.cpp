#include "WindowONEPage.h"

#include <gdiplus.h>
using namespace Gdiplus;

void WindowONEPage::DrawPage(void* DrawDetails)
{
    HDC hDC = (HDC)DrawDetails;
    Graphics graphics(hDC);

    RECT rect;
    SetRect(&rect, 0, 0, 1000, 1000);
    FillRect(hDC, &rect, (HBRUSH)(COLOR_WINDOW + 1));


    SolidBrush  solidBrush(Color(255, 0, 0, 0));
    StringFormat form(StringFormat::GenericTypographic()); // this is needed to prevent characters from including paddinf
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    for (auto& text : TextDivs)
    {
        Font* font;
        FontFamily fontFamily(text->Font.c_str());
        font = new Font(&fontFamily, (REAL)text->FontSize, FontStyleRegular, UnitPixel);
        PointF DrawPos((REAL)text->Left, (REAL)text->Top);
        graphics.DrawString(text->Text.c_str(), (INT)text->Text.length(), font, DrawPos, &form, &solidBrush);

    }

    for (auto& Trace: InkTraces)
    {
        Pen pen(Trace->colour, (REAL)(Trace->thickness_scale / 35.0));
        pen.SetLineJoin(LineJoinRound);
        pen.SetEndCap(LineCapRound);

        Point* Points = (Point*)malloc(Trace->points.size() * sizeof(Point));
        if (Points != NULL)
            for (int i = 0; i < Trace->points.size(); i++)
                Points[i] = Point((int)Trace->points[i].X/20 , (int)Trace->points[i].Y/20);

        graphics.DrawLines(&pen, Points, (int)Trace->points.size());

//        graphics.DrawLine(&pen, { 0, 0 }, Points[0]);
        
        free(Points);
    }


}



