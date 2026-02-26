#pragma once
#include <string>
#include "framework.h"
#include "OneNoteToRM.h"
#include <format>
#include <iostream>
#include <wtypes.h>
#include <tchar.h>
#include "GraphDoc.h"

#include "pugixml.hpp"


typedef struct one_Text {
    std::wstring Font;
    int FontSize;
    int Left;
    int Top;
    std::wstring Text;
} ONE_Text;

typedef struct Ink_Point {
	int X;
	int Y;
	int F;
} INK_Point;

typedef struct Ink_Trace {
	std::vector<INK_Point> points;
	Gdiplus::Color colour;
	UINT32 tool_id = 0;
	DOUBLE thickness_scale = 0;
} INK_Trace;

class ONEPage
{
private:
	std::wstring ParseAttribString(std::wstring Attrib, std::wstring Field);
	int ParseAttribInt(std::wstring Attrib, std::wstring Field);
	void RecurseParseHTMLNode(pugi::xml_node Node, ONE_Text* RootText);
	void ParseInkTrace(std::vector<INK_Point>& points, int NumValuesPerPoint, wchar_t* Data);
	void ParseHTMLNode(pugi::xml_node htmlNode);
	void ParseInkNode(pugi::xml_node InkNode);

protected:
	std::vector<ONE_Text *> TextDivs;
	std::vector<INK_Trace*> InkTraces;

public:
	void LoadPage(std::wstring * Data, std::string& Name);
	virtual void DrawPage(void* DrawDetails);
};
