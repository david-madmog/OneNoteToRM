#include "framework.h"
#include "ToRMOnePage.h"
#include "RMDocFile.h"
#include "ConversionConstants.h"

#pragma warning ( push )
#pragma warning( disable : 4005 26819)
//#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )


void ToRMOnePage::DrawPage(void* DrawDetails)
{
	RMDocFile<WindowRMPage>* DocFile ;
	DocFile = (RMDocFile<WindowRMPage>*) DrawDetails;
	// Create our destination page
	//    OnePage = new WindowONEPage;
	DocFile->AddPage(&RMPage);

	LoadMetaData(DocFile);

	UUID uuid;
	if (UuidCreate(&uuid) != RPC_S_OK)
		return ;
	char* docID = nullptr;
	if (UuidToStringA(&uuid, (RPC_CSTR*)&docID) != RPC_S_OK)
		return ;
	RMPage.m_id = std::string(docID);

	AuthorIds* AI = new AuthorIds;
	AI->AddDefault();
	RMPage.AddBlock(AI);

	MigrationInfo* MI = new MigrationInfo;
	RMPage.AddBlock(MI);

	PageInfo* PI = new PageInfo;
	RMPage.AddBlock(PI);

	SceneInfo* SI = new SceneInfo;
	SI->SetPaperSize(PAGE_SIZE_X, PAGE_SIZE_Y);
	RMPage.AddBlock(SI);

	SceneTree* ST1 = new SceneTree;
	ST1->node_id = { 0, 0 };
	ST1->tree_id = { 2, 20 };
	ST1->parent_id = { 0, 11 };
	RMPage.AddBlock(ST1);

	int ItemId = 30;
	for (auto TextDiv : TextDivs)
	{
		RootText* RT = new RootText;
		RT->pos_x = TextDiv->Left - RM_X_OFFSET;
		RT->pos_y = TextDiv->Top;

		for (auto TextSpan: TextDiv->Texts)
		{
			rm_CRDT_SEQ_ITEM<RM_STRING> RMT;
			RMT.deleted_length = 0;
			RMT.item_id.part1 = 2;
			RMT.item_id.part2 = ItemId;
			std::string * S = new std::string (ws2s(TextSpan.Text));
			RMT.value = (char *) S->c_str();
			RT->texts.push_back(RMT);

			RootTextFormat RTF;
			RTF.charID.part1 = 2;
			RTF.charID.part2 = ItemId;
			RTF.format_code = 1; //!!!
//	  BASIC = 0
//    PLAIN = 1
//    HEADING = 2
//    BOLD = 3
//    BULLET = 4
//    BULLET2 = 5
//    CHECKBOX = 6
//    CHECKBOX_CHECKED = 7
			RT->formats.push_back(RTF);

			ItemId += (int) S->size();
		}

		RMPage.AddBlock(RT);

	}

	SceneTree* ST2 = new SceneTree;
	ST2->node_id = { 0, 11 };
	ST2->tree_id = { 0, 11 };
	ST2->parent_id = { 0, 1 };
	RMPage.AddBlock(ST2);

	TreeNode* TN1 = new TreeNode;
	TN1->node_id = { 0,1 };
	TN1->visible = { {0, 0}, 1 };
	RMPage.AddBlock(TN1);

	TreeNode* TN2 = new TreeNode;
	TN2->node_id = { 0,11 };
	TN2->visible = { {0, 0}, 1 };
	TN2->label.timestamp = { 0, 0 };
	const char* S = "Layer 1";
	TN2->label.value = (char*)S;
	RMPage.AddBlock(TN2);

	SceneGroupItem* SG = new SceneGroupItem;
	SG->item_id = { 0, 13 };
	SG->parent_id = { 0, 1 };
	SG->ID5 = { 0, 11 };
	RMPage.AddBlock(SG);

	int LineID = 14;
	for (auto& InkTrace : InkTraces)
	{
		SceneLineItem* LI = new SceneLineItem;
		LI->item_id = { 0, LineID++ };
		LI->parent_id = { 0,11 };

		//LI->colour = InkTrace->colour;
		LI->tool_id = 17;
		//		LI->thickness_scale = InkTrace->thickness_scale / RM_TO_ONE_THICK_FACTOR;
		LI->thickness_scale = 1 ;

		for (auto& Point : InkTrace->points) {
			SceneLineItem::LineItemPoint* P1 = new SceneLineItem::LineItemPoint;
			P1->width = Point.F / (RM_TO_ONE_LINE_FACTOR * (int)InkTrace->thickness_scale);
			P1->pressure = 255;
			P1->x = (FLOAT) (Point.X / RM_TO_ONE_XY_SCALE_FACTOR) - RM_X_OFFSET;
			P1->y = (FLOAT) Point.Y / RM_TO_ONE_XY_SCALE_FACTOR;
			LI->points.push_back(*P1);
		}

		RMPage.AddBlock(LI);
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

	std::string Data{ R"(
						{
						    "createdTime": "1763459978040",
						        "lastModified" : "1768844812925",
						        "lastOpened" : "1768844757552",
						        "lastOpenedPage" : 0,
						        "new" : false,
						        "parent" : "",
						        "pinned" : false,
						        "source" : "",
						        "type" : "DocumentType",
						        "visibleName" : "Jobs"
						}
					)"
	};

	DocFile->Metadata = json::parse(Data);
}

