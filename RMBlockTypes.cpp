#include "framework.h"
#include "RMBlockTypes.h"
#include "OneNoteToRM.h"

#pragma pack( push, 1)


bool MigrationInfo::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version)
{
	DoLog(typeid(*this).name(), "Migration Info...", LOG_DEBUG_VERBOSE);
	void* Buff_Ptr = (void*)Buff;

	Buff_Ptr = ReadTaggedData(&migration_id, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&is_device, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&Unknown, Buff_Ptr, 3);
	return true;

}

size_t MigrationInfo::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Migration Info...", LOG_DEBUG_VERBOSE);
	
	// compute Size...
	WriteBuffLen = migration_id.SizeOfT() + sizeof(is_device) + sizeof(Unknown) + 2 * SIZE_OF_TAG + sizeof(rm_BlockHead);

	WriteBuff = malloc(WriteBuffLen );
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteTaggedData(&migration_id, Local_Buff, 1);
	Local_Buff = WriteTaggedData(&is_device, Local_Buff, 2);
	Local_Buff = WriteTaggedData(&Unknown, Local_Buff, 3);

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}


SceneTree::SceneTree() {
	is_update = 0;
}

bool SceneTree::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
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
		DoLog(typeid(*this).name(), "Blank Node", LOG_DEBUG_VERBOSE);
		node_id = tree_id;
	}

	sprintf_s(LogBuffer, LB_SIZE, "Layer Def (Scene Tree): Tree ID (%d, %d) Node ID (%d, %d) Parent ID(%d, %d)...",
		tree_id.part1, tree_id.part2, node_id.part1, node_id.part2, parent_id.part1, parent_id.part2
	);
	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
	return true;

};

size_t SceneTree::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Scene Tree...", LOG_DEBUG_VERBOSE);
	RM_CRDT_ID NodeID_To_Use = node_id;
	if (NodeID_To_Use == tree_id)
		NodeID_To_Use = { 0, 0 };

	// Nice and easy... it's always the same size
	WriteBuffLen = sizeof(rm_BlockHead) + tree_id.SizeOfT() + NodeID_To_Use.SizeOfT() + sizeof(is_update) + SIZE_OF_TAG + parent_id.SizeOfT() + SIZE_OF_SUBBLOCK ;

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteTaggedData(&tree_id, Local_Buff, 1);
	Local_Buff = WriteTaggedData(&NodeID_To_Use, Local_Buff, 2);
	Local_Buff = WriteTaggedData(&is_update, Local_Buff, 3);
	Local_Buff = WriteSubblock(Local_Buff, 4, parent_id.SizeOfT());
	Local_Buff = WriteTaggedData(&parent_id, Local_Buff, 1);

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

bool TreeNode::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
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

	sprintf_s(LogBuffer, LB_SIZE, "Layer Names (Tree Node): Node ID (%d, %d) [%s] Anchor ID(%d, %d)...",
		node_id.part1, node_id.part2, label.value, anchor_id.value.part1, anchor_id.value.part2
	);
	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
	return true;

};

size_t TreeNode::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Layer Names (Tree Node)...", LOG_DEBUG_VERBOSE);

	if (anchor_id.value == RM_CRDT_ID{ 0, 0 })
		// just do brief bit
		WriteBuffLen = sizeof(rm_BlockHead) + node_id.SizeOfT(1) + label.SizeOfT(2) + visible.SizeOfT(3);
	else
		WriteBuffLen = sizeof(rm_BlockHead) + node_id.SizeOfT(1) + label.SizeOfT(2) + visible.SizeOfT(3) +
			anchor_id.SizeOfT(7) + anchor_type.SizeOfT(8) + anchor_threshold.SizeOfT(9) + anchor_origin_x.SizeOfT(10);

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead), 2);

	Local_Buff = WriteTaggedData(&node_id, Local_Buff, 1);
	Local_Buff = WriteTaggedData(&label, Local_Buff, 2);
	Local_Buff = WriteTaggedData(&visible, Local_Buff, 3);
	if (anchor_id.value != RM_CRDT_ID{ 0, 0 })
	{
		Local_Buff = WriteTaggedData(&anchor_id, Local_Buff, 7);
		Local_Buff = WriteTaggedData(&anchor_type, Local_Buff, 8);
		Local_Buff = WriteTaggedData(&anchor_threshold, Local_Buff, 9);
		Local_Buff = WriteTaggedData(&anchor_origin_x, Local_Buff, 10);
	}

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

bool SceneTombstoneItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	DoLog(typeid(*this).name(), "Tombstone Item - ignoring", LOG_DEBUG);
	return false;
};

size_t SceneTombstoneItem::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Tombstone Item - ignoring", LOG_DEBUG);
	WriteBuff = NULL;
	WriteBuffLen = 0;
	return 0;
}

bool SceneGlyphItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	DoLog(typeid(*this).name(), "SceneGlyphItem...  TO FINISH", LOG_WARNING);
	void* Buff_Ptr = ReadSceneItemDetails(Buff, ValidLen);

	//# Since reMarkable version 3.6, the start and length are optional
	Buff_Ptr = ReadTaggedDataOptional(&start, Buff_Ptr, 2);
	Buff_Ptr = ReadTaggedDataOptional(&length, Buff_Ptr, 3);

	Buff_Ptr = ReadTaggedDataOptional(&color_id, Buff_Ptr, 4);
	Buff_Ptr = ReadString(&text, Buff_Ptr, 5);

	// ¬¬¬ TO DO
	//	with stream.read_subblock(6) :
	//	num_rects = stream.data.read_varuint()
	//	rectangles = [
	//		si.Rectangle(*[stream.data.read_float64() for _ in range(4)])
	//			for _ in range(num_rects)
	//	]
	return false;

};

size_t SceneGlyphItem::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Scene Glyph Item...   TO DO", LOG_DEBUG_VERBOSE);

	WriteBuffLen = sizeof(rm_BlockHead) + this->SizeOfSceneItemDetails() + 3 * sizeof(UINT32) + 3 * SIZE_OF_TAG
		+ StringWriteSize(text);

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteSceneItemDetails(Local_Buff, 3 * sizeof(UINT32) + 3 * SIZE_OF_TAG);

	//# Since reMarkable version 3.6, the start and length are optional
	Local_Buff = WriteTaggedData(&start, Local_Buff, 2);
	Local_Buff = WriteTaggedData(&length, Local_Buff, 3);

	Local_Buff = WriteTaggedData(&color_id, Local_Buff, 4);
	Local_Buff = WriteString(&text, Local_Buff, 5);

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

bool SceneGroupItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	void* Buff_Ptr = ReadSceneItemDetails(Buff, ValidLen);
	DoLog(typeid(*this).name(), "SceneGroupItem...", LOG_DEBUG_VERBOSE);

	if (Buff_Ptr < Buff + ValidLen) {
		Buff_Ptr = ReadTaggedData(&ID5, Buff_Ptr, 2);
		//	# XXX can there sometimes be something else here ?
	}
	return true;
};

size_t SceneGroupItem::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Scene Group Item...", LOG_DEBUG_VERBOSE);

	WriteBuffLen = sizeof(rm_BlockHead) + this->SizeOfSceneItemDetails() + ID5.SizeOfT() ;

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteSceneItemDetails(Local_Buff, ID5.SizeOfT());  // Passing 3 should be 4
	Local_Buff = WriteTaggedData(&ID5, Local_Buff, 2);  //02 2F 00 0B

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

void* SceneLineItem::LineItemPoint::ParseBuffer(void* Buff, int version)
{
	void* Buff_Ptr = (void*)Buff;

	if (version > 2)
		throw std::domain_error("Unrecognised file version");

	Buff_Ptr = Read(&x, Buff_Ptr);
	Buff_Ptr = Read(&y, Buff_Ptr);

	if (version == 1) {
		FLOAT tmp;
		Buff_Ptr = Read(&tmp, Buff_Ptr);
		speed = (int)(tmp * 4);
		Buff_Ptr = Read(&tmp, Buff_Ptr);
		direction = (int)(255 * tmp / (2 * M_PI));
		Buff_Ptr = Read(&tmp, Buff_Ptr);
		width = (int)(tmp * 4);
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

void* SceneLineItem::LineItemPoint::PrepareWrite(void* Buff_Ptr)
{
	Buff_Ptr = Write(&x, Buff_Ptr);
	Buff_Ptr = Write(&y, Buff_Ptr);

	// We can read in either version 1 or 2, but we only ever write in v2
	Buff_Ptr = Write(&speed, Buff_Ptr);
	Buff_Ptr = Write(&width, Buff_Ptr);
	Buff_Ptr = Write(&direction, Buff_Ptr);
	Buff_Ptr = Write(&pressure, Buff_Ptr);
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

Gdiplus::Color SceneLineItem::colour() {
	switch (color_id)
	{
	case 0:		//BLACK = 0
		return Gdiplus::Color(0, 0, 0);
	case 1:		//GRAY = 1
		return Gdiplus::Color(127, 127, 127);
	case 2:		//WHITE = 2
		return Gdiplus::Color(255, 255, 255);
	case 3:		//YELLOW = 3
		return Gdiplus::Color(255, 255, 0);
	case 4:		//GREEN = 4
		return Gdiplus::Color(0, 255, 0);
	case 5:		//PINK = 5
		return Gdiplus::Color(255, 127, 127);
	case 6:		//BLUE = 6
		return Gdiplus::Color(0, 0, 255);
	case 7:		//RED = 7
		return Gdiplus::Color(255, 0, 0);
	case 8:		//GRAY_OVERLAP = 8
		return Gdiplus::Color(192, 192, 192);
	case 9:		//HIGHLIGHT
		if (HighlightColour == -1)
			return Gdiplus::Color(128, 255, 255, 0);
		else
			// Aha, we do have a HighlightColour - that's the RGB of the colour to use, but we need to override the alpha part
			return (HighlightColour & 0x00FFFFFF) | 0x7F000000 ;
	case 10:		//GREEN_2
		return Gdiplus::Color(0, 192, 0);
	case 11:		//CYAN
		return Gdiplus::Color(0, 255, 255);
	case 12:		//MAGENTA
		return Gdiplus::Color(255,0,255);
	case 13:		//YELLOW_2
		return Gdiplus::Color(192, 192, 0);
	}
	return 0;
}


bool SceneLineItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
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
	{
		bool bT7 = false;
		bool bHC = false;
		Buff_Ptr = ReadTaggedData(&timestamp, Buff_Ptr, 6);
		if (Buff_Ptr < Buff + ValidLen)
		{
			Buff_Ptr = ReadTaggedDataOptional(&Tag7, Buff_Ptr, 7);
			bT7 = true;
		}
		if (Buff_Ptr < Buff + ValidLen)
		{
			Buff_Ptr = ReadTaggedDataOptional(&HighlightColour, Buff_Ptr, 8);
			bHC = true;
		}

		sprintf_s(LogBuffer, LB_SIZE, "Line Item: ToolID: %d ColorID: %d thickness_scale: %lf starting Length: %f Tag7: %c(%d, %d), Tag8 %c:%X", 
			tool_id, color_id, thickness_scale, starting_length, bT7?'Y':'N', Tag7.part1, Tag7.part2, bHC ? 'Y' : 'N', HighlightColour);
		DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
	}

	sprintf_s(LogBuffer, LB_SIZE, "Line Item v%d (%d points)...", version, numPoints);
	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);

	return (numPoints > 0);
};

size_t SceneLineItem::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Scene Line Item...", LOG_DEBUG_VERBOSE);

	size_t LinesLen = LineItemPoint::point_serialized_size(2) * points.size();
	size_t SceneItemWithoutLinesLen = 2 * sizeof(UINT32) + sizeof(DOUBLE) + sizeof(FLOAT) + 4 * SIZE_OF_TAG + SIZE_OF_SUBBLOCK
		+ timestamp.SizeOfT()
		//		+ Tag7.SizeOfT()
		//		+ sizeof(UINT32) + SIZE_OF_TAG + 1 /* +1 is for tag above 8*/;
		;

	if (Tag7 != RM_CRDT_ID{ 255, 255 })
		SceneItemWithoutLinesLen += Tag7.SizeOfT();
	if (HighlightColour != 0xFFFFFFFF) 
		SceneItemWithoutLinesLen += sizeof(UINT32) + SIZE_OF_TAG + 1 /* +1 is for tag above 8*/;


	WriteBuffLen = sizeof(rm_BlockHead) + this->SizeOfSceneItemDetails() + LinesLen + SceneItemWithoutLinesLen;

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead), 2);

	sprintf_s(LogBuffer, LB_SIZE, "Line Item (buffer size %d)...", (int)(WriteBuffLen - sizeof(rm_BlockHead)));
	DoLog(typeid(*this).name(), LogBuffer, LOG_WARNING);

	Local_Buff = WriteSceneItemDetails(Local_Buff, LinesLen + SceneItemWithoutLinesLen);
	Local_Buff = WriteTaggedData(&tool_id, Local_Buff, 1);
	Local_Buff = WriteTaggedData(&color_id, Local_Buff, 2);
	Local_Buff = WriteTaggedData(&thickness_scale, Local_Buff, 3);
	Local_Buff = WriteTaggedData(&starting_length, Local_Buff, 4);

	Local_Buff = WriteSubblock(Local_Buff, 5, LinesLen);
	for (auto &P : points) {
		Local_Buff = P.PrepareWrite(Local_Buff);
	}
	
	Local_Buff = WriteTaggedData(&timestamp, Local_Buff, 6);
	if (Tag7 != RM_CRDT_ID{ 255, 255 })
		Local_Buff = WriteTaggedData(&Tag7, Local_Buff, 7);
	if (HighlightColour != 0xFFFFFFFF)
		Local_Buff = WriteTaggedData(&HighlightColour, Local_Buff, 8);

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

bool SceneTextItem::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version)
{
	DoLog(typeid(*this).name(), "SceneTextItem...", LOG_DEBUG_VERBOSE);
	void* Buff_Ptr = ReadSceneItemDetails(Buff, ValidLen);

	Buff_Ptr = ReadStringWithFormat(&text, &fmt, Buff_Ptr, 6);
	//	# It seems that formats are stored on empty strings, so it's one or the other
	//	if fmt is not None:
	//if text :
	//	_logger.error(
	//		"Unhandled combined text and format: %s, %s", text, fmt
	//	)
	//	value = fmt
	//else:
	//value = text
	//	else:
	//value = ""
	return false;
};

size_t SceneTextItem::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Scene Text Item...", LOG_DEBUG_VERBOSE);

	WriteBuffLen = sizeof(rm_BlockHead) + this->SizeOfSceneItemDetails() /* + &text, &fmt ¬¬¬ */;

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteSceneItemDetails(Local_Buff, WriteBuffLen - (sizeof(rm_BlockHead) + this->SizeOfSceneItemDetails()));
//	Local_Buff = WriteTaggedData(&ID5, Local_Buff, 2);

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

bool RootText::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	//	"Parse root text block."

	DoLog(typeid(*this).name(), "RootText...", LOG_DEBUG_VERBOSE);

	RM_CRDT_ID ID;
	void* Buff_Ptr = ReadTaggedData(&ID, (void *)Buff, 1);
//		assert block_id == CrdtId(0, 0)

	Buff_Ptr = ReadSubblock(Buff_Ptr, 2);

	int NumSubBlocks = 0;
	// Is this meant to be twice???
	Buff_Ptr = ReadSubblock(Buff_Ptr, 1);
	Buff_Ptr = ReadSubblock(Buff_Ptr, 1);
	Buff_Ptr = ReadVarUINT(&NumSubBlocks, Buff_Ptr);

	for (int i = 0; i < NumSubBlocks; i++) {
		struct rm_CRDT_SEQ_ITEM<RM_STRING> text;
		Buff_Ptr = ReadTextItem(&(text), Buff_Ptr);
		texts.push_back(text);
		sprintf_s(LogBuffer, LB_SIZE, "...Text ID (%d, %d) Left (%d, %d) Right (%d, %d) :%s",
			text.item_id.part1, text.item_id.part2, text.left_id.part1, text.left_id.part2, 
			text.right_id.part1,text.right_id.part2, text.value
		);
		DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
	}
//
//		# Formatting
	Buff_Ptr = ReadSubblock(Buff_Ptr, 2);
	Buff_Ptr = ReadSubblock(Buff_Ptr, 1);
	Buff_Ptr = ReadVarUINT(&NumSubBlocks, Buff_Ptr);
	for (int i = 0; i < NumSubBlocks; i++) {
		struct RootTextFormat format;
		UINT8 c;
		Buff_Ptr = Read(&format.charID, Buff_Ptr);
		Buff_Ptr = ReadTaggedData(&format.timestamp, Buff_Ptr, 1);
		Buff_Ptr = ReadSubblock(Buff_Ptr, 2);
		Buff_Ptr = Read(&c, Buff_Ptr);
		//	# XXX not sure what this is format ?
					//	assert c == 17
		Buff_Ptr = Read(&format.format_code, Buff_Ptr);
					//	try :
					//	format_type = si.ParagraphStyle(format_code)
					//	except ValueError :
					//_logger.warning("Unrecognised text format code %d.", format_code)
					//	_logger.debug(
					//		"Unrecognised text format code %d at position %d.",
					//		format_code,
					//		stream.data.tell(),
					//		)
					//	format_type = si.ParagraphStyle.PLAIN  # fallback
				//		)
				// 
		formats.push_back(format);
		sprintf_s(LogBuffer, LB_SIZE, "...FORMAT ID (%d, %d) Code %d",
			format.charID.part1, format.charID.part2, format.format_code
		);
		DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);
	}
//
//		# Last section
	Buff_Ptr = ReadSubblock(Buff_Ptr, 3);
	//# "pos_x" and "pos_y" from ddvk? Gives negative number -- possibly could
	//# be bounding box ?
	Buff_Ptr = Read(&pos_x, Buff_Ptr);
	Buff_Ptr = Read(&pos_y, Buff_Ptr);

//# "width" from ddvk
	Buff_Ptr = ReadTaggedData(&width, Buff_Ptr, 4);
	return true;
};

size_t RootText::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Root Text...", LOG_DEBUG_VERBOSE);

	size_t TextsLen = VarUINTLen((int)texts.size());
	for (auto& text : texts) {
		TextsLen += text.SizeOfTWithoutTemplateItem();
		TextsLen += StringWriteSize(text.value);
	}

	size_t FormatsLen = VarUINTLen((int)formats.size());
	for (auto& format: formats) {
		FormatsLen += format.SizeOfT();
	}
	RM_CRDT_ID BlankID = { 0, 0 };

	WriteBuffLen = sizeof(rm_BlockHead) + TextsLen + FormatsLen + 5 * SIZE_OF_SUBBLOCK + BlankID.SizeOfT()
		+ SIZE_OF_SUBBLOCK + 2 * sizeof(DOUBLE) + SIZE_OF_TAG + sizeof(FLOAT);

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));

	Local_Buff = WriteTaggedData(&BlankID, (void*)Local_Buff, 1);
	//		assert block_id == CrdtId(0, 0)

	Local_Buff = WriteSubblock(Local_Buff, 2, TextsLen + FormatsLen + 4 * SIZE_OF_SUBBLOCK);

	// Is this meant to be twice???
	Local_Buff = WriteSubblock(Local_Buff, 1, TextsLen + SIZE_OF_SUBBLOCK );
	Local_Buff = WriteSubblock(Local_Buff, 1, TextsLen);
	Local_Buff = WriteVarUINT((int)texts.size(), Local_Buff);
	for (auto& text : texts) {
		Local_Buff = WriteTextItem(&text, Local_Buff);
	}

	//
	//		# Formatting
	Local_Buff = WriteSubblock(Local_Buff, 2, FormatsLen + SIZE_OF_SUBBLOCK);
	Local_Buff = WriteSubblock(Local_Buff, 1, FormatsLen);
	Local_Buff = WriteVarUINT((int)formats.size(), Local_Buff);
	for (auto &format: formats)
	{
		UINT8 c=17;
		Local_Buff = Write(&format.charID, Local_Buff);
		Local_Buff = WriteTaggedData(&format.timestamp, Local_Buff, 1);
		Local_Buff = WriteSubblock(Local_Buff, 2, 2 * sizeof(UINT8));
		Local_Buff = Write(&c, Local_Buff);
		Local_Buff = Write(&format.format_code, Local_Buff);
	}
	//
	//		# Last section
	Local_Buff = WriteSubblock(Local_Buff, 3, 2 * sizeof(DOUBLE));
	Local_Buff = Write(&pos_x, Local_Buff);
	Local_Buff = Write(&pos_y, Local_Buff);
	Local_Buff = WriteTaggedData(&width, Local_Buff, 4);

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}


bool AuthorIds::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	void* Buff_Ptr = (void*)Buff;
	int UUID_Len;
	UUID* AuthorUUID;
	UINT16 AuthorID;

	int numSubBlocks;
	Buff_Ptr = ReadVarUINT(&numSubBlocks, Buff_Ptr);
	sprintf_s(LogBuffer, LB_SIZE, "Author ID's (%d)...", numSubBlocks);
	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG_VERBOSE);

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
	return true;
};

void AuthorIds::AddDefault()
{
	UUID* AuthorUUID = new UUID;
	if (UuidCreate(AuthorUUID) == RPC_S_OK)
		Authors.push_back(*AuthorUUID);

}


size_t AuthorIds::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Author ID's...", LOG_DEBUG_VERBOSE);

	WriteBuffLen = sizeof(rm_BlockHead) + VarUINTLen((int)Authors.size()) + 
		Authors.size() * (SIZE_OF_SUBBLOCK + 1 + 16 + sizeof(UINT16)) ;

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteVarUINT((int)Authors.size(), Local_Buff);  

	UINT16 i = 1;
	for (UUID AuthorUUID : Authors) {
		Local_Buff = WriteSubblock(Local_Buff, 0, VarUINTLen(16) + 16 + sizeof(UINT16));
		Local_Buff = WriteVarUINT(16, Local_Buff);
		Local_Buff = Write((UINT8*)&AuthorUUID, Local_Buff, 16);
		Local_Buff = Write(&i, Local_Buff);
	}

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

bool PageInfo::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) {
	DoLog(typeid(*this).name(), "Page Info...", LOG_DEBUG_VERBOSE);
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
	return true;
};

size_t PageInfo::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Scene Group Item...", LOG_DEBUG_VERBOSE);

	WriteBuffLen = sizeof(rm_BlockHead) + 5 * (sizeof(UINT32) + SIZE_OF_TAG);

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteTaggedData(&loads_count, Local_Buff, 1);
	Local_Buff = WriteTaggedData(&merges_count, Local_Buff, 2);
	Local_Buff = WriteTaggedData(&text_chars_count, Local_Buff, 3);
	Local_Buff = WriteTaggedData(&text_lines_count, Local_Buff, 4);
	Local_Buff = WriteTaggedData(&Unknown, Local_Buff, 5);

	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

bool SceneInfo::ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version)
{
	DoLog(typeid(*this).name(), "Scene Info...", LOG_DEBUG_VERBOSE);
	void* Buff_Ptr = (void*)Buff;

	Buff_Ptr = ReadTaggedData(&currentLayer, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&BackgroundVisible, Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&root_document_visible, Buff_Ptr, 3);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadIntPair(paper_size, Buff_Ptr, 5);

	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadSubblock(Buff_Ptr, 6);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&ID3, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadSubblock(Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = Read(Magic1, Buff_Ptr, 32);

//		6c  28 00 00 00   -> Subblock
//			1F 00 00      -> ID
//			2C 20 00 00 00     --> ???
//				0x20 lots of 00

	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadSubblock(Buff_Ptr, 7);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = Read(&Magic2a, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = Read(&Magic2b, Buff_Ptr, 1);

//		7c  10 00 00 00   -> SubBlock
//			00 00 00 00  00 F0 95 40  00 00 00 00  00 40 9D 40 --> ?? Pair of doubles?

	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadSubblock(Buff_Ptr, 8);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadTaggedData(&ID4, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = ReadSubblock(Buff_Ptr, 2);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = Read(&Magic3a, Buff_Ptr, 1);
	if (Buff_Ptr < Buff + ValidLen)
		Buff_Ptr = Read(&Magic3b, Buff_Ptr, 1);
//		8c 01  18 00 00 00  --> 8C = length = 18   --> SubBlock
//			1F 00 00 -> ID
//			2C 10 00 00 00 
//				00 00 00 00  00 F0 95 40  00 00 00 00  00 40 9D 40 
		 


	return true;
};

size_t SceneInfo::PrepareWrite()
{
	DoLog(typeid(*this).name(), "Scene Group Item...", LOG_DEBUG_VERBOSE);

	WriteBuffLen = sizeof(rm_BlockHead) + currentLayer.SizeOfT() + BackgroundVisible.SizeOfT() 
		+ root_document_visible.SizeOfT() + SIZE_OF_SUBBLOCK + 2 * sizeof(UINT32)
		+ ID3.SizeOfT() + sizeof(Magic1) + 2 * SIZE_OF_SUBBLOCK
		+ 2 * sizeof(DOUBLE) + SIZE_OF_SUBBLOCK
		+ ID4.SizeOfT(8) + 2 * sizeof(DOUBLE) + 2 * SIZE_OF_SUBBLOCK
		;

	WriteBuff = malloc(WriteBuffLen);
	void* Local_Buff = WriteBlockHead(WriteBuff, WriteBuffLen - sizeof(rm_BlockHead));
	Local_Buff = WriteTaggedData(&currentLayer, Local_Buff, 1);
	Local_Buff = WriteTaggedData(&BackgroundVisible, Local_Buff, 2);
	Local_Buff = WriteTaggedData(&root_document_visible, Local_Buff, 3);
	Local_Buff = WriteIntPair(paper_size, Local_Buff, 5);

	Local_Buff = WriteSubblock(Local_Buff, 6, ID3.SizeOfT() + sizeof(Magic1) + SIZE_OF_SUBBLOCK );
	Local_Buff = WriteTaggedData(&ID3, Local_Buff, 1);
	Local_Buff = WriteSubblock(Local_Buff, 2, sizeof(Magic1));
	Local_Buff = Write(Magic1, Local_Buff, 32);

	Local_Buff = WriteSubblock(Local_Buff, 7, 2 * sizeof(DOUBLE));
	Local_Buff = Write(&Magic2a, Local_Buff, 1);
	Local_Buff = Write(&Magic2b, Local_Buff, 1);

	Local_Buff = WriteSubblock(Local_Buff, 8, ID4.SizeOfT() + 2 * sizeof(DOUBLE) + SIZE_OF_SUBBLOCK);
	Local_Buff = WriteTaggedData(&ID4, Local_Buff, 1);
	Local_Buff = WriteSubblock(Local_Buff, 2, 2 * sizeof(DOUBLE));
	Local_Buff = Write(&Magic3a, Local_Buff, 1);
	Local_Buff = Write(&Magic3b, Local_Buff, 1);




	assert((char*)Local_Buff == (char*)WriteBuff + WriteBuffLen);
	return WriteBuffLen;
}

#pragma pack( pop )

