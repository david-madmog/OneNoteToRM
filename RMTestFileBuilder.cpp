#pragma comment(lib, "rpcrt4.lib")  // UuidCreate 
#include <type_traits>
#include <iostream>
#include "RMTestFileBuilder.h"

// We need to include possible types so compiler knows what instantiations we need
#include "WindowRMPage.h"
template int RMTestFileBuilder<WindowRMPage>::Build();


template<class PageType> 
int RMTestFileBuilder<PageType>::Build()
{
	LoadMetaData();

	UUID uuid;
	if (UuidCreate(&uuid) != RPC_S_OK)
		return 0;
	char* docID = nullptr;
	if (UuidToStringA(&uuid, (RPC_CSTR*)&docID) != RPC_S_OK)
		return 0;

	PageType* Page = new PageType(docID);
	this->Pages.push_back(Page);

	AuthorIds* AI = new AuthorIds;
	AI->AddDefault();
	Page->AddBlock(AI);

	MigrationInfo * MI = new MigrationInfo;
	Page->AddBlock(MI);

	PageInfo * PI = new PageInfo;
	Page->AddBlock(PI);

	SceneInfo * SI = new SceneInfo;
	Page->AddBlock(SI);

	SceneTree* ST = new SceneTree;
	ST->node_id = { 0,11 };
	ST->parent_id = { 0,1 };
	Page->AddBlock(ST);

	TreeNode* TN1 = new TreeNode;
	TN1->node_id = { 0,1 };
	Page->AddBlock(TN1);

	TreeNode* TN2 = new TreeNode;
	TN2->node_id = { 0,11 };
	Page->AddBlock(TN2);

	SceneGroupItem * SG = new SceneGroupItem;
	SG->item_id = { 0, 13 };
	SG->parent_id = { 0, 1 };
	Page->AddBlock(SG);

	SceneLineItem * LI = new SceneLineItem;
	LI->item_id = { 0,14 };
	LI->parent_id = { 0,11 };

	SceneLineItem::LineItemPoint * P1 = new SceneLineItem::LineItemPoint;
	P1->width = 1;
	P1->x = 10;
	P1->y = 10;
	LI->points.push_back(*P1);
	SceneLineItem::LineItemPoint * P2 = new SceneLineItem::LineItemPoint;
	P2->width = 1;
	P2->x = 100;
	P2->y = 100;
	LI->points.push_back(*P2);
	
	Page->AddBlock(LI);

	return (int)this->Pages.size() ;
}

template<class PageType>
void RMTestFileBuilder<PageType>::LoadMetaData() {
	
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

	this->Metadata = json::parse(Data);
}

