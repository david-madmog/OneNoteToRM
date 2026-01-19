#include "RMBlock.h"
#include "DOCXToRM.h"


RMBlock::RMBlock() {
	Class = BlockClass::BCUnknown;
}

void * RMBlock::ReadVarUINT(int * data, void * Buff_Ptr) {
	//Read a varuint from the data stream."""
	int shift = 0;
	int result = 0;
	int i;
	UINT8* Local_Ptr = (UINT8*)Buff_Ptr;
	while (true) {
		i = *Local_Ptr;
		Local_Ptr++;
		//i = ord(self.read_bytes(1))
		result |= (i & 0x7F) << shift;
		shift += 7;
		if (!(i & 0x80))
			break;
	}
	*data = result;
	return Local_Ptr;
}


void * RMBlock::ReadTag(RM_Tag* tag, void* Buff_Ptr, int index, TagTypeEnum TagType) {
	int x;
	
	Buff_Ptr = ReadVarUINT(&x, Buff_Ptr);

	//First part is an index number that identifies if this is the right data we're expecting
	tag->index = x >> 4;

	// Second part is a tag type that identifies what kind of data it is
	tag->TagType = (TagTypeEnum)(x & 0xF);
	//	return index, tag_type

	if (tag->index != index) {
		char LogBuff[1024];
		sprintf_s(LogBuff, "Expected index %d, got %d, at position %d", index, tag->index, 0);
		throw incorrect_tag(LogBuff);
	}
	if (tag->TagType != TagType) {
		char LogBuff[1024];
		sprintf_s(LogBuff, "Expected tag type %d, got %d, at position %d", TagType, tag->TagType, 0);
		throw incorrect_tag(LogBuff);
	}

	return Buff_Ptr;
};

void* RMBlock::ReadString(RM_STRING* data, void * Buff_Ptr, int index) {
//	"""Read a standard string block."""
	Buff_Ptr = ReadSubblock(Buff_Ptr, index);
	int string_length;
	RM_BOOL is_ascii;

	Buff_Ptr = ReadVarUINT(&string_length, Buff_Ptr);
	Buff_Ptr = Read(&is_ascii, Buff_Ptr);
	//# XXX not sure if this is right meaning ?

	if (! is_ascii)
	{
		char LogBuff[1024];
		sprintf_s(LogBuff, "String type not ASCII");
		throw std::range_error::range_error(LogBuff);
	}	
	
	//assert string_length + 2 <= block_info.size
	*data = new char[string_length+1];
	Buff_Ptr = Read((UINT8 *)*data, Buff_Ptr, string_length);
	(*data)[string_length] = 0;
	return Buff_Ptr;
}

///////////////////////
// Overloaded set of generic read functions
void* RMBlock::Read(UINT8* data, void* Buff_Ptr, int count) {
	UINT8* Local_Ptr;

	memcpy(data, Buff_Ptr, count);
	Local_Ptr = (UINT8*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Read(UINT16* data, void* Buff_Ptr, int count) {
	UINT16* Local_Ptr;

	memcpy(data, Buff_Ptr, count * sizeof(UINT16));
	Local_Ptr = (UINT16*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Read(FLOAT* data, void* Buff_Ptr, int count) {
	FLOAT* Local_Ptr;

	memcpy(data, Buff_Ptr, count * sizeof(FLOAT));
	Local_Ptr = (FLOAT*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}


void* RMBlock::ReadIntPair(UINT32* Ints, void* Buff_Ptr, int index) {
	UINT32* Local_Ptr;

	Local_Ptr = (UINT32*)ReadSubblock(Buff_Ptr, index);
	Ints[0] = *(Local_Ptr++);
	Ints[1] = *(Local_Ptr++);
	return (void*)Local_Ptr;
}


///////////////////////
// Overloaded set of main read TAG functions

void* RMBlock::ReadTaggedData(RM_CRDT_ID* ID, void* Buff_Ptr, int index) {
	RM_Tag tag;
	UINT8* Local_Ptr;

	Local_Ptr = (UINT8*)ReadTag(&tag, Buff_Ptr, index, TagTypeEnum::ID);
	ID->part1 = *Local_Ptr;
	Local_Ptr++;

	Buff_Ptr = ReadVarUINT(&ID->part2, (void*)Local_Ptr);
	return Buff_Ptr;
}

void* RMBlock::ReadTaggedData(RM_BOOL* Bool, void* Buff_Ptr, int index) {
	RM_Tag tag;
	RM_BOOL* Local_Ptr;

	Local_Ptr = (RM_BOOL*)ReadTag(&tag, Buff_Ptr, index, TagTypeEnum::Byte1);
	*Bool = *Local_Ptr;
	Local_Ptr++;

	return Local_Ptr;
}

void* RMBlock::ReadTaggedData(RM_LWW_ID* ID, void* Buff_Ptr, int index)
{
	Buff_Ptr = ReadSubblock(Buff_Ptr, index);
	Buff_Ptr = ReadTaggedData(&ID->timestamp, Buff_Ptr, 1);
	Buff_Ptr = ReadTaggedData(&ID->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::ReadTaggedData(RM_LWW_Bool* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = ReadSubblock(Buff_Ptr, index);
	Buff_Ptr = ReadTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = ReadTaggedData(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::ReadTaggedData(RM_LWW_Byte* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = ReadSubblock(Buff_Ptr, index);
	Buff_Ptr = ReadTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = ReadTaggedData(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::ReadTaggedData(RM_LWW_String* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = ReadSubblock(Buff_Ptr, index);
	Buff_Ptr = ReadTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = ReadString(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::ReadTaggedData(RM_LWW_Float* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = ReadSubblock(Buff_Ptr, index);
	Buff_Ptr = ReadTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = ReadTaggedData(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::ReadTaggedData(UINT32* data, void* Buff_Ptr, int index)
{
	RM_Tag tag;
	UINT32* Local_Ptr;

	Local_Ptr = (UINT32*)ReadTag(&tag, Buff_Ptr, index, TagTypeEnum::Byte4);
	*data = *Local_Ptr;
	Local_Ptr++;

	return Local_Ptr;
}

void* RMBlock::ReadTaggedData(FLOAT* data, void* Buff_Ptr, int index)
{
	RM_Tag tag;
	FLOAT* Local_Ptr;

	Local_Ptr = (FLOAT*)ReadTag(&tag, Buff_Ptr, index, TagTypeEnum::Byte4);
	*data = *Local_Ptr;
	Local_Ptr++;

	return Local_Ptr;
}

void* RMBlock::ReadTaggedData(DOUBLE* data, void* Buff_Ptr, int index)
{
	RM_Tag tag;
	DOUBLE* Local_Ptr;

	Local_Ptr = (DOUBLE*)ReadTag(&tag, Buff_Ptr, index, TagTypeEnum::Byte8);
	*data = *Local_Ptr;
	Local_Ptr++;

	return Local_Ptr;
}

void* RMBlock::ReadTaggedDataOptional(UINT32* data, void* Buff_Ptr, int index) {
	
	try {
		Buff_Ptr = ReadTaggedData(data, Buff_Ptr, index);
	}
	catch (incorrect_tag)
	{
		// OK - in this case it's fine, we just didn't have the thing we thought
		*data = 0;
	}

	return Buff_Ptr;
}



void* RMBlock::ReadSubblock(void* Buff_Ptr, int index) {
	UINT32 subblock_length;
	return (ReadSubblock(Buff_Ptr, index, &subblock_length));
}

void* RMBlock::ReadSubblock(void* Buff_Ptr, int index, UINT32 * subblock_length) {
		//def read_subblock(self, index: int)->Iterator[SubBlockInfo]:
	//Read a subblock length and return `SubBlockInfo` as context object.
	//	Checks that the correct length has been read at the end of the with
	//	block.
	UINT32* Local_Ptr ;

	RM_Tag Tag;
	Local_Ptr = (UINT32 *)ReadTag(&Tag, Buff_Ptr, index, TagTypeEnum::Length4);

	*subblock_length = *Local_Ptr;
	Local_Ptr++;
	//	self._check_position(subblock)
	// Idea is we do something with this to check the block is the right size

	return Local_Ptr;
};


RMSceneItemBlock::RMSceneItemBlock()
{
	Class = BlockClass::Item;
	deleted_length = 0;
}


void* RMSceneItemBlock::ReadSceneItemDetails(const unsigned char* Buff, size_t ValidLen)
{
	char MessageBuffer[1024];
	void* Buff_Ptr = (void *)Buff;
	UINT8 SubblockType;

	Buff_Ptr = ReadTaggedData(&parent_id, Buff_Ptr, 1);
	Buff_Ptr = ReadTaggedData(&item_id, Buff_Ptr, 2);
	Buff_Ptr = ReadTaggedData(&left_id, Buff_Ptr, 3);
	Buff_Ptr = ReadTaggedData(&right_id, Buff_Ptr, 4);
	Buff_Ptr = ReadTaggedData(&deleted_length, Buff_Ptr, 5);
	if (Buff_Ptr < Buff + ValidLen)
	{
		Buff_Ptr = ReadSubblock(Buff_Ptr, 6);
		Buff_Ptr = Read(&SubblockType, Buff_Ptr);
	}

	//	assert item_type == subclass.ITEM_TYPE
	//	value = subclass.value_from_stream(stream)
	//	# Keep known extra data
	//	extra_data = block_info.extra_data
	//	else:
	//value = None
	//	extra_data = b""

	//	return subclass(
	//		parent_id,
	//		CrdtSequenceItem(item_id, left_id, right_id, deleted_length, value),
	//		extra_data = extra_data,
	//		)

	if (deleted_length > 0)
		sprintf_s(MessageBuffer, "<B> Scene Item (deleted %d)...", deleted_length );
	else
		sprintf_s(MessageBuffer, "<B> Scene Item: Parent (%d, %d) ID (%d, %d) Left (%d, %d) Right (%d, %d) Deleted Len %d...",
			parent_id.part1, parent_id.part2, item_id.part1, item_id.part2, left_id.part1, left_id.part2, right_id.part1, right_id.part2, deleted_length
		);
	DoLog(MessageBuffer);

	return Buff_Ptr;
}

