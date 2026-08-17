#include "pch.h"
#include "ToRMOnePage.h"
#include "RMDocFile.h"
#include "ConversionConstants.h"

/*******************************************************************************

	ToRMPOnePage.cpp

	See header for documentation

	(C) David Poirier 2026

********************************************************************************/

#pragma warning ( push )
#pragma warning( disable : 4005 26819)
//#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )


ToRMOnePage::ToRMOnePage()
{
	RMPage = new WindowRMPage;
}

UINT8  ToRMOnePage::DetermineRMFormatCode(ONE_TextSpan TextSpan)
{
	//# Based on a rm file having 4 anchors based on the line height I was able to find a value of
	//    # 69.5, but decided on 70 (to keep integer values)
	//    si.ParagraphStyle.PLAIN: 70,
	//    si.ParagraphStyle.BULLET : 35,
	//    si.ParagraphStyle.BULLET2 : 35,
	//    si.ParagraphStyle.BOLD : 70,
	//    si.ParagraphStyle.HEADING : 150,
	//    si.ParagraphStyle.CHECKBOX : 35,
	//    si.ParagraphStyle.CHECKBOX_CHECKED : 35,
	//	  BASIC = 0
	//    PLAIN = 1
	//    HEADING = 2
	//    BOLD = 3
	//    BULLET = 4
	//    BULLET2 = 5
	//    CHECKBOX = 6
	//    CHECKBOX_CHECKED = 7
	if (TextSpan.FontSize < 24)
	{
		return 4;
	}
	else if (TextSpan.FontSize < 48)
	{
//		if (TextSpan.)
		return 1;
	}
	else
		return 2;



}

void ToRMOnePage::DrawPage(void* DrawDetails)
{
	RMDocFile<WindowRMPage>* DocFile ;
	DocFile = (RMDocFile<WindowRMPage>*) DrawDetails;
	// Create our destination page
	//    OnePage = new WindowONEPage;
	DocFile->AddPage(RMPage);

	LoadMetaData(DocFile);

	UUID uuid;
	if (UuidCreate(&uuid) != RPC_S_OK)
		return ;
	char* docID = nullptr;
	if (UuidToStringA(&uuid, (RPC_CSTR*)&docID) != RPC_S_OK)
		return ;
	RMPage->m_id = std::string(docID);

	AuthorIds* AI = new AuthorIds;
	AI->AddDefault();
	RMPage->AddBlock(AI);

	MigrationInfo* MI = new MigrationInfo;
	RMPage->AddBlock(MI);

	PageInfo* PI = new PageInfo;
	RMPage->AddBlock(PI);

	SceneInfo* SI = new SceneInfo;
	SI->SetPaperSize(PAGE_SIZE_X, PAGE_SIZE_Y);
	RMPage->AddBlock(SI);

	SceneTree* ST1 = new SceneTree;
	ST1->node_id = { 0, 0 };
	ST1->tree_id = { 2, 20 };
	ST1->parent_id = { 0, 11 };
	RMPage->AddBlock(ST1);

	int ItemId = 30;
	for (auto TextDiv : TextDivs)
	{
		RootText* RT = new RootText;
		//		RT->pos_x = TextDiv->Left - RM_X_OFFSET;
		RT->pos_x = TextDiv->Left - RM_X_OFFSET;
		RT->pos_y = TextDiv->Top;
		RT->width = PAGE_SIZE_X;

		for (auto TextSpan: TextDiv->Texts)
		{
			rm_CRDT_SEQ_ITEM<RM_STRING> RMT;
			RMT.deleted_length = 0;
			RMT.item_id.part1 = 2;
			RMT.item_id.part2 = ItemId;
			std::string * S = new std::string (ws2s(TextSpan.Text));
			S->append("\n");
			RMT.value = (char *) S->c_str();
			RT->texts.push_back(RMT);

			RootTextFormat RTF;
			RTF.charID.part1 = 2;
			RTF.charID.part2 = ItemId;
			RTF.format_code = DetermineRMFormatCode(TextSpan);
			RT->formats.push_back(RTF);

			std::wostringstream LB;
			LB << L"Text " << RMT.value << L": Format " << RTF.format_code << L" at ID:" << RTF.charID;
			DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG);

			ItemId += (int) S->size();
		}

		RMPage->AddBlock(RT);

	}

	SceneTree* ST2 = new SceneTree;
	ST2->node_id = { 0, 11 };
	ST2->tree_id = { 0, 11 };
	ST2->parent_id = { 0, 1 };
	RMPage->AddBlock(ST2);

	TreeNode* TN1 = new TreeNode;
	TN1->node_id = { 0,1 };
	TN1->visible = { {0, 0}, 1 };
	RMPage->AddBlock(TN1);

	TreeNode* TN2 = new TreeNode;
	TN2->node_id = { 0,11 };
	TN2->visible = { {0, 0}, 1 };
	TN2->label.timestamp = { 0, 0 };
	TN2->anchor_origin_x = { { 0, 0 } , ONE_TO_RM_X_OFFSET };
	const char* S = "Layer 1";
	TN2->label.value = (char*)S;
	RMPage->AddBlock(TN2);

	SceneGroupItem* SG = new SceneGroupItem;
	SG->item_id = { 0, 13 };
	SG->parent_id = { 0, 1 };
	SG->ID5 = { 0, 11 };
	RMPage->AddBlock(SG);

	int LineID = 14;
	for (auto& InkTrace : InkTraces)
	{
		SceneLineItem* LI = new SceneLineItem;
		LI->item_id = { 0, LineID++ };
		LI->parent_id = { 0,11 };

		LI->tool_id = 17;
		LI->SetColor(InkTrace->colour); // May also change tool ID

		//		LI->thickness_scale = InkTrace->thickness_scale / RM_TO_ONE_THICK_FACTOR;
		LI->thickness_scale = 1 ;

		for (auto& Point : InkTrace->points) {
			SceneLineItem::LineItemPoint* P1 = new SceneLineItem::LineItemPoint;
			//			P1->width = Point.F / (RM_TO_ONE_LINE_FACTOR * (int)InkTrace->thickness_scale);
			//			P1->width = (int)(Point.F * InkTrace->thickness_scale / RM_TO_ONE_LINE_FACTOR) ;
			P1->width = (int)((Point.F / RM_TO_ONE_PRESSURE_FACTOR) + (InkTrace->thickness_scale / RM_TO_ONE_LINE_FACTOR));
			if (P1->width == 0)
				P1->width = 1;
			P1->pressure = 255;

			P1->x = (FLOAT) (Point.X / RM_TO_ONE_XY_SCALE_FACTOR) - RM_X_OFFSET + ONE_TO_RM_X_OFFSET;
			P1->y = (FLOAT) Point.Y / RM_TO_ONE_XY_SCALE_FACTOR - LINES_Y_START;
			LI->points.push_back(*P1);
		}

		RMPage->AddBlock(LI);
	}


	return;
}

/*
* 
	BALLPOINT_1 = 2
	BALLPOINT_2 = 15
	CALIGRAPHY = 21
	ERASER = 6
	ERASER_AREA = 8
	FINELINER_1 = 4
	FINELINER_2 = 17
	HIGHLIGHTER_1 = 5
	HIGHLIGHTER_2 = 18
	MARKER_1 = 3
	MARKER_2 = 16
	MECHANICAL_PENCIL_1 = 7
	MECHANICAL_PENCIL_2 = 13
	PAINTBRUSH_1 = 0
	PAINTBRUSH_2 = 12
	PENCIL_1 = 1
	PENCIL_2 = 14
	SHADER = 23

*/

void ToRMOnePage::LoadMetaData(RMDocFile<WindowRMPage>* DocFile) {

	std::string Data{ R"({		"createdTime": "0",
						        "lastOpenedPage" : 1,
						        "new" : false,
						        "parent" : "",
						        "pinned" : false,
						        "source" : "",
						        "type" : "DocumentType"
						})"
	};

	DocFile->Metadata = json::parse(Data, nullptr, false, true);

	time_t Now;
	time(&Now);
	std::ostringstream NowString;
	NowString << (Now * 1000);

	DocFile->Metadata["lastModified"] = NowString.str();
	DocFile->Metadata["lastOpened"] = NowString.str();

}

