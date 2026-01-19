#define _USE_MATH_DEFINES
#include <math.h>
#include "RMBlockTypes.h"
#include "DOCXToRM.h"

void MigrationInfo::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version)
{
	DoLog("<B> Migration Info...");
	void* Buff_Ptr = (void*)Buff;

	Buff_Ptr = ReadTaggedData(&migration_id, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&is_device, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&Unknown, Buff_Ptr, 3);

}

SceneTree::SceneTree() {
	Class = BlockClass::SceneTreeClass;
	is_update = 0;
}

void SceneTree::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	char MessageBuffer[1024];
	void* Buff_Ptr = (void*)Buff;

	//# XXX not sure what the difference is.This "tree_id" is used as the
	//	# plain "Id" in the SceneTree.NodeMap in ddvk's reader. If the parent_id
	//	# is equal to the root_id(1, 1), this node represents a layer.
	Buff_Ptr = ReadTaggedData(&tree_id, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&node_id, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&is_update, Buff_Ptr, 3);
	if (Buff_Ptr < Buff + ValidLen) {
		Buff_Ptr = ReadSubblock(Buff_Ptr, 4);
		Buff_Ptr = ReadTaggedData(&parent_id, Buff_Ptr, 1);
		//	# XXX can there sometimes be something else here ?
	}

	//¬¬ So, if node Id is blank, we should use tree ID as the node ID
	if (node_id == RM_CRDT_ID{0, 0}) {
		DoLog("<B> Blank Node");
		node_id = tree_id;
	}


//	if (is_update)
//		sprintf_s(MessageBuffer, "<B> Layer Def (Scene Tree): Update...");
//	else
		sprintf_s(MessageBuffer, "<B> Layer Def (Scene Tree): Tree ID (%d, %d) Node ID (%d, %d) Parent ID(%d, %d)...",
			tree_id.part1, tree_id.part2, node_id.part1, node_id.part2, parent_id.part1, parent_id.part2
		);
	DoLog(MessageBuffer);

};

TreeNode::TreeNode() {
	Class = BlockClass::TreeNodeClass;
}

void TreeNode::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	char MessageBuffer[1024];
	void* Buff_Ptr = (void*)Buff;

	Buff_Ptr = ReadTaggedData(&node_id, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&label, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&visible, Buff_Ptr, 3);

	//	# XXX this may need to be generalised for other examples
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&anchor_id, Buff_Ptr, 7);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&anchor_type, Buff_Ptr, 8);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&anchor_threshold, Buff_Ptr, 9);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&anchor_origin_x, Buff_Ptr, 10);

	sprintf_s(MessageBuffer, "<B> Layer Names (Tree Node): Node ID (%d, %d) [%s] Anchor ID(%d, %d)...",
		node_id.part1, node_id.part2, label.value, anchor_id.value.part1, anchor_id.value.part2
	);
	DoLog(MessageBuffer);

};


void SceneTombstoneItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	DoLog("<B> Tombstone Item - ignoring");
};

void SceneGlyphItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	DoLog("<B> #### SceneGlyphItem...  TO FINISH");
	void* Buff_Ptr = ReadSceneItemDetails(Buff, ValidLen);

	//# Since reMarkable version 3.6, the start and length are optional
	Buff_Ptr = ReadTaggedDataOptional(&start, Buff_Ptr, 2);
	Buff_Ptr = ReadTaggedDataOptional(&length, Buff_Ptr, 3);

	Buff_Ptr = ReadTaggedDataOptional(&color_id, Buff_Ptr, 4);
	Buff_Ptr = ReadString(&text, Buff_Ptr, 5);
	//	color_id = stream.read_int(4)
	//	color = si.PenColor(color_id)
	//	text = stream.read_string(5)

	//	if length is None :
	//length = len(text)

	//	# Note : the decoded text length is not always the same as the length in the
	//	# glyph range...
	//	if len(text) != length:
	//_logger.debug(
	//	"GlyphRange text length %d != length value %d: %r",
	//	len(text),
	//	length,
	//	text,
	//	)

	//	with stream.read_subblock(6) :
	//	num_rects = stream.data.read_varuint()
	//	rectangles = [
	//		si.Rectangle(*[stream.data.read_float64() for _ in range(4)])
	//			for _ in range(num_rects)
	//	]


};

void SceneGroupItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	void* Buff_Ptr = ReadSceneItemDetails(Buff, ValidLen);
	DoLog("<B> SceneGroupItem...");

	if (Buff_Ptr < Buff + ValidLen) {
		Buff_Ptr = ReadTaggedData(&ID5, Buff_Ptr, 2);
		//	# XXX can there sometimes be something else here ?
	}

};

void* SceneLineItem::LineItemPoint::ParseBuffer(void* Buff, int version)
{
	void* Buff_Ptr = (void*)Buff;

	if (version > 2)
		throw std::domain_error("Unrecognised file version");

	Buff_Ptr = Read(&x, Buff_Ptr);
	Buff_Ptr = Read(&y, Buff_Ptr);

	if (version == 1) {
		FLOAT tmp;
		//		# speed = int(round(d.read_float32() * 4))
		Buff_Ptr = Read(&tmp, Buff_Ptr);
		speed = (int)(tmp * 4);
		//		direction = 255 * d.read_float32() / (math.pi * 2)
		Buff_Ptr = Read(&tmp, Buff_Ptr);
		direction = (int)(255 * tmp / (2 * M_PI));
		//		width = int(round(d.read_float32() * 4))
		Buff_Ptr = Read(&tmp, Buff_Ptr);
		width = (int)(tmp * 4);
		//		pressure = d.read_float32() * 255
		Buff_Ptr = Read(&tmp, Buff_Ptr);
		pressure = (int)(tmp * 255);
	}
	else {
		Buff_Ptr = Read(&speed, Buff_Ptr);
		Buff_Ptr = Read(&width, Buff_Ptr);
		Buff_Ptr = Read(&direction, Buff_Ptr);
		Buff_Ptr = Read(&pressure, Buff_Ptr);
	}
	return Buff_Ptr;
}

int SceneLineItem::LineItemPoint::point_serialized_size(int version)
{
	if (version == 1)
		return 0x18;
	else if (version == 2)
		return 0x0E;
	else
		throw std::domain_error("Unrecognised file version");
}

COLORREF SceneLineItem::colour() {
	switch (color_id)
	{
	case 0: //BLACK = 0
		return RGB(0, 0, 0);
	case 1:		//GRAY = 1
		return RGB(127, 127, 127);
	case 2:		//WHITE = 2
		return RGB(255, 255, 255);

	case 3:		//YELLOW = 3
		return RGB(255, 255, 0);
	case 4:		//GREEN = 4
		return RGB(0, 255, 0);
	case 5:		//PINK = 5
		return RGB(255, 127, 127);

	case 6:		//BLUE = 6
		return RGB(0, 0, 255);
	case 7:		//RED = 7
		return RGB(255, 0, 0);

	case 8:		//GRAY_OVERLAP = 8
		return RGB(192, 192, 192);
	}
}


void SceneLineItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	char MessageBuffer[1024];
	int numPoints = 0;

	void* Buff_Ptr = ReadSceneItemDetails(Buff, ValidLen);

	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&tool_id, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&color_id, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&thickness_scale, Buff_Ptr, 3);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&starting_length, Buff_Ptr, 4);

	UINT32 data_length;
	if (Buff_Ptr < Buff + ValidLen) {
		Buff_Ptr = ReadSubblock(Buff_Ptr, 5, &data_length);

		if (data_length % LineItemPoint::point_serialized_size(version) != 0)
		{
			throw std::range_error("Point Size incorrect");
		}
		numPoints = data_length / LineItemPoint::point_serialized_size(version);

		for (int i = 0; i < numPoints; i++)
		{
			LineItemPoint* P = new LineItemPoint;
			Buff_Ptr = P->ParseBuffer(Buff_Ptr, version);
			points.push_back(*P);
		}
	}
	//		# XXX unused
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&timestamp, Buff_Ptr, 6);

	sprintf_s(MessageBuffer, "<B> Line Item (%d points)...", numPoints);
	DoLog(MessageBuffer);
};

void SceneTextItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version)
{
	DoLog("<B> ##### SceneTextItem TO DO...");
	void* Buff_Ptr = ReadSceneItemDetails(Buff, ValidLen);
};

RootText::RootText() {
	Class = BlockClass::RootTextClass;
}

void RootText::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	DoLog("<B> RootText ##### TO DO...");
};


void AuthorIds::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	char MessageBuffer[1024];

	void* Buff_Ptr = (void*)Buff;
	int UUID_Len;
	UUID* AuthorUUID;
	UINT16 AuthorID;

	int numSubBlocks;
	Buff_Ptr = ReadVarUINT(&numSubBlocks, Buff_Ptr);
	sprintf_s(MessageBuffer, "<B> Author ID's (%d)...", numSubBlocks);
	DoLog(MessageBuffer);

	for (int i = 0; i < numSubBlocks; i++) {
		Buff_Ptr = ReadSubblock(Buff_Ptr, 0);
		Buff_Ptr = ReadVarUINT(&UUID_Len, Buff_Ptr);
		if (UUID_Len != 16)
			throw std::range_error::range_error("Expected UUID length to be 16 bytes");
		AuthorUUID = new UUID;
		Buff_Ptr = Read((UINT8*)AuthorUUID, Buff_Ptr, 16);
		Buff_Ptr = Read(&AuthorID, Buff_Ptr);
		Authors.push_back(*AuthorUUID);
	}
};


void PageInfo::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	DoLog("<B> Page Info...");
	void* Buff_Ptr = (void*)Buff;

	Buff_Ptr = ReadTaggedData(&loads_count, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&merges_count, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&text_chars_count, Buff_Ptr, 3);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&text_lines_count, Buff_Ptr, 4);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&Unknown, Buff_Ptr, 5);
};

void SceneInfo::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version)
{
	DoLog("<B> Scene Info...");
	void* Buff_Ptr = (void*)Buff;

	Buff_Ptr = ReadTaggedData(&currentLayer, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&BackgroundVisible, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&root_document_visible, Buff_Ptr, 3);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadIntPair(paper_size, Buff_Ptr, 5);
};

