#pragma once
#include <vector>
#include "RMBlock.h"



class MigrationInfo : public RMBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
private:
	RM_CRDT_ID migration_id;
	RM_BOOL is_device;
	RM_BOOL Unknown;
};

class SceneTree : public RMBlock {
public:
	SceneTree();
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
	RM_CRDT_ID tree_id;
	RM_CRDT_ID node_id;
	RM_CRDT_ID parent_id;
private:
	RM_BOOL is_update = 0;
};

class TreeNode : public RMBlock {
public: 
	TreeNode();
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
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
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
};

class SceneGlyphItem : public RMSceneItemBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
private:
	UINT32 start;
	UINT32 length;

	UINT32 color_id;
//	color = si.PenColor(color_id)
	RM_STRING text;
};

class SceneGroupItem : public RMSceneItemBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
private:
	UINT8 magic;
	RM_CRDT_ID ID5;
};


class SceneLineItem : public RMSceneItemBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);

	class LineItemPoint {
	public:
		static int point_serialized_size(int version = 2);
		void* ParseBuffer(void* Buff, int version);
		FLOAT x;
		FLOAT y;

		UINT16 speed;
		UINT8 direction;
		UINT16 width;
		UINT8 pressure;
	};

	std::vector<LineItemPoint> points;
	COLORREF colour();
	UINT32 tool_id;
	DOUBLE thickness_scale;

private:
	UINT32 color_id;
	FLOAT	starting_length;
	RM_CRDT_ID timestamp;
};

class SceneTextItem : public RMSceneItemBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
};

class RootText : public RMBlock {
public:
	RootText();
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
};

class AuthorIds : public RMBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
private:
	std::vector<UUID> Authors;
};

class PageInfo : public RMBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);
private:
	UINT32 loads_count;
	UINT32 merges_count;
	UINT32 text_chars_count;
	UINT32 text_lines_count;
	UINT32 Unknown;
};

class SceneInfo : public RMBlock {
public:
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version);

private:
	rm_LWW_ID currentLayer ;
	rm_LWW_Bool BackgroundVisible ;
	rm_LWW_Bool root_document_visible;
	UINT32 paper_size[2] = { 0,0 };
};


