#pragma once
#include <vector>
#include <gdiplus.h>
#include "RMBlock.h"

/*******************************************************************************

	RMBlockTypes.h

	Header for generic ReMarkable file format blocks

	(C) David Poirier 2026

	Based on knowledge from:
	* Rick Lupton - https://github.com/ricklupton rmscene
	* ddvk - https://github.com/ddvk

********************************************************************************

  So, the ReMarkable file represents one page. It consists of a series of blocks 
of data of different types. I have defined a class for each of these blocks. Method
is to look at the (common) header of each block, determine it's type, then create 
an instance of the appropriate class and tell it to parse it's block.

These blocks/classes can be lumped into the following categories:
* Limited Page blocks:
	- MigrationInfo
	- AuthorIds
	- PageInfo
	- SceneInfo

* Hierarchy/tree organisation
	- TreeNode (AKA "Layer Names") - These are parents (or root ancestors) of scene items. 
		Each has an Anchor Hash which refers to one of the text sequences in the root text block
	- SceneTree (AKA "Layer Definition") - These seem to mirror the tree nodes (same Hash's)
	- SceneGroupItem - Also inherits from "Item Block", so has a "Parent Hash", can act as an 
		intermediate node between lines etc. and the tree nodes.

* Blocks containing actual stuff to draw/render
	- SceneTombstoneItem (Seems to be placeholder for deleted stuff?)
	- SceneGlyphItem (Never actually seen one of these, but Rick seems to think they exist)
	- SceneLineItem (Most of the actual stuff)
	- SceneTextItem... TO DO
  These all inherit from "Item Block", which means that as well as the item Hash, they have a 
"Parent Hash", which represents the grouping item and ultimately determines where they will be 
drawn on the page.

* Root text block
  This seems to contain the actual text on the page. It consists of a number of strings, and 
  an associated "start Hash", then subsequent anchor Hash's refer to subsequent characters in the 
  string which are the base points of the referred items


*******************************************************************************/

class MigrationInfo : public RMBlock {
public:
	~MigrationInfo() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_MigrationInfo; };
private:
	RM_CRDT_ID migration_id = {1, 1};
	RM_BOOL is_device = 1;
	RM_BOOL Unknown = 0;
};

class SceneTree : public RMBlock {
public:
	SceneTree();
	~SceneTree() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_SceneTree; };
	RM_CRDT_ID tree_id;
	RM_CRDT_ID node_id;
	RM_CRDT_ID parent_id;
private:
	RM_BOOL is_update = 0;
};

class TreeNode : public RMBlock {
public: 
	~TreeNode() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_TreeNode; };
	RM_CRDT_ID node_id;
	RM_LWW_String label;
	RM_LWW_Bool visible;
	RM_LWW_ID anchor_id;
	RM_LWW_Byte anchor_type;
	RM_LWW_Float anchor_threshold;
	RM_LWW_Float anchor_origin_x;
private:

};

class SceneTombstoneItem : public RMBlock {
public:
	~SceneTombstoneItem() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_SceneTombstoneItem; };
};

class SceneGlyphItem : public RMSceneItemBlock {
public:
	~SceneGlyphItem() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_SceneGlyphItem; };
	inline BT_SubBlockType SubBlockType() { return SBT_GlyphBlock; };
private:
	UINT32 start = 0;
	UINT32 length = 0;

	UINT32 color_id = 0; //	color = si.PenColor(color_id)
	RM_STRING text = NULL;
};

class SceneGroupItem : public RMSceneItemBlock {
public:
	~SceneGroupItem() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_SceneGroupItem; };
	inline BT_SubBlockType SubBlockType() { return SBT_GroupBlock; };
	RM_CRDT_ID ID5 = { 0, 0 };
private:
	UINT8 magic = 0;
};


class SceneLineItem : public RMSceneItemBlock {
public:
	~SceneLineItem() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_SceneLineItem; };
	inline BT_SubBlockType SubBlockType() { return SBT_LineBlock; };

	class LineItemPoint {
	public:
		static int point_serialized_size(int version = 2);
		void* ParseBuffer(void* Buff, int version);
		void* PrepareWrite(void* Buff);
		FLOAT x;
		FLOAT y;

		UINT16 speed;
		UINT8 direction;
		UINT16 width;
		UINT8 pressure;
	};

	std::vector<LineItemPoint> points;
	Gdiplus::Color colour();
	void SetColor(Gdiplus::Color InkColour);
	UINT32 tool_id = 0;
	DOUBLE thickness_scale = 0;
private:
	UINT32 color_id = 0;
	FLOAT	starting_length = 0;
	RM_CRDT_ID timestamp = {0, 0};
	RM_CRDT_ID Tag7 = { 0xFF, 0xFF };
	UINT32 HighlightColour = -1;

};

class SceneTextItem : public RMSceneItemBlock {
public:
	~SceneTextItem() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_SceneTextItem; };
	inline BT_SubBlockType SubBlockType() { return SBT_TextBlock; };
private:
	UINT32 fmt = 0; 
	RM_STRING text = NULL;
};

struct RootTextFormat {
	RM_CRDT_ID charID = { 0, 0 };
	RM_CRDT_ID timestamp = {0, 0};
	UINT8 format_code = 0;
	UINT8 code2 = 0;
	UINT32 code3 = 0;
	size_t SizeOf() const { return charID.SizeOf() + timestamp.SizeOf() + 2 * sizeof(UINT8) + SIZE_OF_SUBBLOCK; }
	size_t SizeOfT(int index = 0) const { return charID.SizeOf() + timestamp.SizeOfT() + 2 * sizeof(UINT8) + SIZE_OF_SUBBLOCK + (index > 7 ? 1 : 0); }
};

class RootText : public RMBlock {
public:
	~RootText() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_RootText; };
	std::vector<struct rm_CRDT_SEQ_ITEM<RM_STRING>> texts;
	std::vector<struct RootTextFormat> formats;
	DOUBLE pos_x = 0;
	DOUBLE pos_y = 0;
	FLOAT width = 0;

private:
};

class AuthorIds : public RMBlock {
public:
	~AuthorIds() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_AuthorIds; };
	void AddDefault();
private:
	std::vector<UUID> Authors;
};

class PageInfo : public RMBlock {
public:
	~PageInfo() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_PageInfo; };
private:
	UINT32 loads_count = 1;
	UINT32 merges_count = 1;
	UINT32 text_chars_count = 0;
	UINT32 text_lines_count = 0;
	UINT32 Unknown = 0;
};

class SceneInfo : public RMBlock {
public:
	~SceneInfo() {};
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	size_t PrepareWrite();
	inline BT_BlockType BlockType() { return BT_SceneInfo; };
	void SetPaperSize(int X, int Y) { paper_size[0] = X; paper_size[1] = Y; }
	void GetPaperSize(unsigned int& X, unsigned int& Y) const { X = paper_size[0]; Y = paper_size[1]; }
private:
	rm_LWW_ID currentLayer{ {0, 0}, {0, 1} };
	rm_LWW_Bool BackgroundVisible = { {0, 0}, 1 };
	rm_LWW_Bool root_document_visible = { {0, 0}, 1 };
	UINT32 paper_size[2] = { 1,1 };

	RM_CRDT_ID ID3;
	UINT8 Magic1[32] = {};
	DOUBLE Magic2a = 0;
	DOUBLE Magic2b = 0;
	RM_CRDT_ID ID4;
	DOUBLE Magic3a = 0;
	DOUBLE Magic3b = 0;
};


