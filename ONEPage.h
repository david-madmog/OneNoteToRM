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


typedef struct one_TextSpan {
	std::wstring Font;
	int FontSize = 0;
	std::wstring Text;
} ONE_TextSpan;

typedef struct one_Text {
    int Left = 0;
    int Top = 0;
	std::vector<ONE_TextSpan> Texts;
} ONE_Text;

typedef struct Ink_Point {
	int X;
	int Y;
	int F;
} INK_Point;

typedef struct Ink_Trace {
	std::vector<INK_Point> points;
	Gdiplus::Color colour;
	std::wstring tool_id;
	std::wstring trace_id;
	DOUBLE thickness_scale = 0;

	bool operator<(Ink_Trace b) { return tool_id < b.tool_id; };
	bool operator==(Ink_Trace b) { return tool_id == b.tool_id; };
} INK_Trace;

class ONEPage
{
private:
	std::wstring ParseAttribString(std::wstring Attrib, std::wstring Field);
	int ParseAttribInt(std::wstring Attrib, std::wstring Field);
	void ParseInkTrace(std::vector<INK_Point>& points, int NumValuesPerPoint, wchar_t* Data);
	void ParseHTMLNode(pugi::xml_node htmlNode);
	void ParseInkNode(pugi::xml_node InkNode);
	std::wstring SaveText();
	void SaveInkTrace(pugi::xml_node node, INK_Trace * Trace);
	void CreateChildWithAttrs(pugi::xml_node node, const std::wstring& Name, int NumPairs, ...);
	std::wstring SaveInk();

protected:
	std::vector<ONE_Text *> TextDivs;
	std::vector<INK_Trace*> InkTraces;
	std::wstring PageTitle = L"";

public:
	void LoadPage(std::wstring * Data, std::string& Name);
	std::wstring* SavePage(std::wstring& MultipartBoundary);
	virtual void DrawPage(void* DrawDetails);
};
