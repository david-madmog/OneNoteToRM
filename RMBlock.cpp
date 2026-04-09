#include "framework.h"
#include "RMBlock.h"
#include "OneNoteToRM.h"


//std::wostream& operator<< (std::wostream& stream, const RM_CRDT_ID& RHS) {
//	stream << L"{" << RHS.part1 << L"-" << RHS.part2 << L"}";
//	return stream;
//}

RMBlock::RMBlock() {
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
		if (shift > 21) // We limit ourselves to 4 bytes...
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
	//	char LogBuff[1024];
	//	sprintf_s(LogBuff, "Expected index %d, got %d, at position %d", index, tag->index, 0);
		throw incorrect_tag("Index not as expected");
	}
	if (tag->TagType != TagType) {
	//	char LogBuff[1024];
	//	sprintf_s(LogBuff, "Expected tag type %d, got %d, at position %d", TagType, tag->TagType, 0);
		throw incorrect_tag("TagType not as expected");
	}

	return Buff_Ptr;
};

void* RMBlock::ReadStringWithFormat(RM_STRING* data, UINT32* fmt, void* Buff_Ptr, int index) {
	//"""Read a string block with formatting."""
	// First, read standard string block

	try {
		Buff_Ptr = ReadString(data, Buff_Ptr, index);
	}
	catch (incorrect_tag& ex)
	{
		// tag not there... just carry on
		DoLog("ReadStringWithFormat", ex.what(), LOG_DEBUG_VERBOSE);
	}

	try {
		// See if the tag is there... without advancing!
		RM_Tag Tag;
		ReadTag(&Tag, Buff_Ptr, 2, TagTypeEnum::Byte4);

		// if tag IS there, we will not have an exception, so we can read it
		Buff_Ptr = ReadTaggedData(fmt, Buff_Ptr, 2);
	}
	catch (incorrect_tag& ex)
	{
		// tag not there... just carry on
		DoLog("ReadStringWithFormat", ex.what(), LOG_DEBUG_VERBOSE);
	}
	
	return Buff_Ptr;
}

void* RMBlock::ReadString(RM_STRING* data, void* Buff_Ptr, int index) {
	//	"""Read a standard string block."""
	Buff_Ptr = ReadSubblock(Buff_Ptr, index);
	int string_length;
	RM_BOOL is_ascii;

	Buff_Ptr = ReadVarUINT(&string_length, Buff_Ptr);
	Buff_Ptr = Read(&is_ascii, Buff_Ptr);

	if (! is_ascii)
		throw std::range_error::range_error("String type not ASCII");
	
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

void* RMBlock::Read(DOUBLE* data, void* Buff_Ptr, int count) {
	DOUBLE* Local_Ptr;

	memcpy(data, Buff_Ptr, count * sizeof(DOUBLE));
	Local_Ptr = (DOUBLE*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Read(RM_CRDT_ID* ID, void* Buff_Ptr, int count) {
	UINT8* Local_Ptr;

	Local_Ptr = (UINT8*)Buff_Ptr;
	ID->part1 = *Local_Ptr;
	Local_Ptr++;

	Buff_Ptr = ReadVarUINT(&ID->part2, (void*)Local_Ptr);
	return Buff_Ptr;
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

void* RMBlock::ReadTaggedDataOptional(UINT8* data, void* Buff_Ptr, int index) {

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

void* RMBlock::ReadTaggedDataOptional(RM_CRDT_ID* data, void* Buff_Ptr, int index) {

	try {
		Buff_Ptr = ReadTaggedData(data, Buff_Ptr, index);
	}
	catch (incorrect_tag)
	{
		// OK - in this case it's fine, we just didn't have the thing we thought
		*data = {0, 0};
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

void* RMBlock::ReadTextItem(struct rm_CRDT_SEQ_ITEM<RM_STRING>* data, void* Buff_Ptr)
{
	UINT32 fmt = 0;

	Buff_Ptr = ReadSubblock((void*)Buff_Ptr, 0);

	Buff_Ptr = ReadTaggedData(&data->item_id, Buff_Ptr, 2);
	Buff_Ptr = ReadTaggedData(&data->left_id, Buff_Ptr, 3);
	Buff_Ptr = ReadTaggedData(&data->right_id, Buff_Ptr, 4);
	Buff_Ptr = ReadTaggedData(&data->deleted_length, Buff_Ptr, 5);
	Buff_Ptr = ReadStringWithFormat(&data->value, &fmt, Buff_Ptr, 6);
			//# It seems that formats are stored on empty strings, so it's one or the other
			//if fmt is not None:
			//	if text :
			//		_logger.error("Unhandled combined text and format: %s, %s", text, fmt)
			//		value = fmt
			//	else:
			//		value = text
			//else:
			//	value = ""

	return Buff_Ptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t RMBlock::PrepareWrite() {
	return 0; // Will be overridden by derived classes
}

void* RMBlock::WriteBlock(void* Buff)
{
	// So, the subclass should have nicely prepared the buffer in WriteBuff for length WriteBuffLen
	if (!WriteBuff)
		throw std::logic_error("Write Not Prepared");

	memcpy(Buff, WriteBuff, WriteBuffLen);
	Buff = (void*)((char*)Buff + WriteBuffLen);
	free(WriteBuff);
	WriteBuff = NULL;
	WriteBuffLen = 0;

	return Buff; 
}

void* RMBlock::WriteBlockHead(void* Buff_Ptr, size_t block_length, unsigned char version)
{
	rm_BlockHead BH{};

	BH.magic1 = 0;
	BH.MinVersion = version;
	BH.CurrentVersion = version;
	BH.BlockType = BlockType();
	BH.len_body = (UINT32)block_length;

	memcpy(Buff_Ptr, &BH, sizeof(BH));
	Buff_Ptr = (void*)((UINT8*)Buff_Ptr + sizeof(BH));

	return Buff_Ptr;
}


void* RMBlock::WriteVarUINT(int data, void* Buff_Ptr) {
	//Write a varuint to the data stream."""

	unsigned int uData = (unsigned int)data;

	UINT8* Local_Ptr = (UINT8*)Buff_Ptr;
	int to_write;
	while (true) {
		to_write = uData & 0x7F;
		uData >>= 7;
		if (uData)
			*(Local_Ptr++) = to_write | 0x80;
		else {
			*(Local_Ptr++) = to_write;
			break;
		}
	}
	return Local_Ptr;
}

size_t RMBlock::VarUINTLen(int data)
{
	if (data < 0)
		return 5;
	if (data < 0x80)
		return 1;
	if (data < 0x4000)
		return 2;
	if (data < 0x200000)
		return 3;
	return 4;
}

void* RMBlock::WriteTag(void* Buff_Ptr, int index, TagTypeEnum TagType) {
	int x = index << 4 | int(TagType);

	Buff_Ptr = WriteVarUINT(x, Buff_Ptr);
	return Buff_Ptr;
};

void* RMBlock::WriteStringWithFormat(RM_STRING* data, UINT32* fmt, void* Buff_Ptr, int index) {   // despite the name, we just ignore the format!
	Buff_Ptr = WriteString(data, Buff_Ptr, index);
//	Buff_Ptr = WriteTaggedData(fmt, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::WriteString(RM_STRING* data, void* Buff_Ptr, int index) { 
	//	"""Read a standard string block."""
	void* Local_Ptr = Buff_Ptr;
	size_t string_length = *data?strlen(*data):0;

	Buff_Ptr = WriteSubblock(Buff_Ptr, index, StringWriteSize(*data) - SIZE_OF_SUBBLOCK);

	RM_BOOL is_ascii = true;

	Buff_Ptr = WriteVarUINT((int) string_length, Buff_Ptr);
	Buff_Ptr = Write(&is_ascii, Buff_Ptr);
	Buff_Ptr = Write((UINT8 *)*data, Buff_Ptr, (int)string_length);

	return Buff_Ptr;
}

size_t RMBlock::StringWriteSize(RM_STRING data) 
{
	size_t string_length = (data ? strlen(data) : 0) ;
	return SIZE_OF_SUBBLOCK + VarUINTLen((int)string_length) + sizeof(RM_BOOL) + string_length;
}

///////////////////////
// Overloaded set of generic write functions
void* RMBlock::Write(UINT8* data, void* Buff_Ptr, int count) {
	UINT8* Local_Ptr;

	memcpy(Buff_Ptr, data, count);
	Local_Ptr = (UINT8*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Write(UINT16* data, void* Buff_Ptr, int count) {
	UINT16* Local_Ptr;

	memcpy(Buff_Ptr, data, count * sizeof(UINT16));
	Local_Ptr = (UINT16*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Write(UINT32* data, void* Buff_Ptr, int count) {
	UINT32* Local_Ptr;

	memcpy(Buff_Ptr, data, count * sizeof(UINT32));
	Local_Ptr = (UINT32*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Write(FLOAT* data, void* Buff_Ptr, int count) {
	FLOAT* Local_Ptr;

	memcpy(Buff_Ptr, data, count * sizeof(FLOAT));
	Local_Ptr = (FLOAT*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Write(DOUBLE* data, void* Buff_Ptr, int count) {
	DOUBLE* Local_Ptr;

	memcpy(Buff_Ptr, data, count * sizeof(DOUBLE));
	Local_Ptr = (DOUBLE*)Buff_Ptr;
	Local_Ptr += count;
	return (void*)Local_Ptr;
}

void* RMBlock::Write(RM_CRDT_ID* ID, void* Buff_Ptr, int count) {
	Buff_Ptr = Write(&ID->part1, Buff_Ptr);
	Buff_Ptr = WriteVarUINT(ID->part2, Buff_Ptr);
	
	return Buff_Ptr;
}

void* RMBlock::WriteIntPair(UINT32* Ints, void* Buff_Ptr, int index) {
	Buff_Ptr = WriteSubblock(Buff_Ptr, index, (UINT32)(2 * sizeof(UINT32)));
	Buff_Ptr = Write(Ints, Buff_Ptr, 2);
	return Buff_Ptr;
}


///////////////////////
// Overloaded set of main read TAG functions

void* RMBlock::WriteTaggedData(RM_CRDT_ID* ID, void* Buff_Ptr, int index) {
	Buff_Ptr = WriteTag(Buff_Ptr, index, TagTypeEnum::ID);
	Buff_Ptr = Write(ID, Buff_Ptr);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(RM_BOOL* Bool, void* Buff_Ptr, int index) {
	Buff_Ptr = WriteTag(Buff_Ptr, index, TagTypeEnum::Byte1);
	Buff_Ptr = Write(Bool, Buff_Ptr);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(RM_LWW_ID* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = WriteSubblock(Buff_Ptr, index, data->SizeOf() - SIZE_OF_SUBBLOCK);
	Buff_Ptr = WriteTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = WriteTaggedData(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(RM_LWW_Bool* data, void* Buff_Ptr, int index) 
{
	Buff_Ptr = WriteSubblock(Buff_Ptr, index, data->SizeOf() - SIZE_OF_SUBBLOCK);
	Buff_Ptr = WriteTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = WriteTaggedData(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(RM_LWW_Byte* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = WriteSubblock(Buff_Ptr, index, data->SizeOf() - SIZE_OF_SUBBLOCK);
	Buff_Ptr = WriteTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = WriteTaggedData(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(RM_LWW_String* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = WriteSubblock(Buff_Ptr, index, data->SizeOfT() - SIZE_OF_SUBBLOCK);  
	Buff_Ptr = WriteTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = WriteString(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(RM_LWW_Float* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = WriteSubblock(Buff_Ptr, index, data->SizeOf() - SIZE_OF_SUBBLOCK);
	Buff_Ptr = WriteTaggedData(&data->timestamp, Buff_Ptr, 1);
	Buff_Ptr = WriteTaggedData(&data->value, Buff_Ptr, 2);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(UINT32* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = WriteTag(Buff_Ptr, index, TagTypeEnum::Byte4);
	Buff_Ptr = Write(data, Buff_Ptr);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(FLOAT* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = WriteTag(Buff_Ptr, index, TagTypeEnum::Byte4);
	Buff_Ptr = Write(data, Buff_Ptr);
	return Buff_Ptr;
}

void* RMBlock::WriteTaggedData(DOUBLE* data, void* Buff_Ptr, int index)
{
	Buff_Ptr = WriteTag(Buff_Ptr, index, TagTypeEnum::Byte8);
	Buff_Ptr = Write(data, Buff_Ptr);
	return Buff_Ptr;
}

//void* RMBlock::ReadTaggedDataOptional(UINT32* data, void* Buff_Ptr, int index) {
//
//	try {
//		Buff_Ptr = ReadTaggedData(data, Buff_Ptr, index);
//	}
//	catch (incorrect_tag)
//	{
//		// OK - in this case it's fine, we just didn't have the thing we thought
//		*data = 0;
//	}
//
//	return Buff_Ptr;
//}
//

void* RMBlock::WriteSubblock(void* Buff_Ptr, int index, size_t subblock_length) {
	Buff_Ptr = WriteTag(Buff_Ptr, index, TagTypeEnum::Length4);
	Buff_Ptr = Write((UINT32 *) & subblock_length, Buff_Ptr);
	return Buff_Ptr;
}
	
void* RMBlock::WriteTextItem(struct rm_CRDT_SEQ_ITEM<RM_STRING>* data, void* Buff_Ptr) 
{
	UINT32 fmt = 0;

	Buff_Ptr = WriteSubblock((void*)Buff_Ptr, 0, data->SizeOfTWithoutTemplateItem() + StringWriteSize(data->value) - SIZE_OF_SUBBLOCK);

	Buff_Ptr = WriteTaggedData(&data->item_id, Buff_Ptr, 2);
	Buff_Ptr = WriteTaggedData(&data->left_id, Buff_Ptr, 3);
	Buff_Ptr = WriteTaggedData(&data->right_id, Buff_Ptr, 4);
	Buff_Ptr = WriteTaggedData(&data->deleted_length, Buff_Ptr, 5);
	Buff_Ptr = WriteStringWithFormat(&data->value, &fmt, Buff_Ptr, 6);

	return Buff_Ptr;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////

RMSceneItemBlock::RMSceneItemBlock()
{
	deleted_length = 0;
}


void* RMSceneItemBlock::ReadSceneItemDetails(const unsigned char* Buff, size_t ValidLen)
{
	void* Buff_Ptr = (void*)Buff;
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
	{
		std::wostringstream LB;
		LB << L"Scene Item (deleted " << deleted_length << L")...";
		DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
	}
	else
	{
		std::wostringstream LB;
		LB << L"Scene Item: Parent " << parent_id << L" ID " << item_id;
		DoLog(typeid(*this).name(), LB.str(), LOG_DEBUG_VERBOSE);
	}

	return Buff_Ptr;
}

void* RMSceneItemBlock::WriteSceneItemDetails(void* Buff_Ptr, size_t SubblockLen)
{
	UINT8 SubblockType = SubBlockType();   //// WTF is the Subblock type???

	Buff_Ptr = WriteTaggedData(&parent_id, Buff_Ptr, 1);
	Buff_Ptr = WriteTaggedData(&item_id, Buff_Ptr, 2);
	Buff_Ptr = WriteTaggedData(&left_id, Buff_Ptr, 3);
	Buff_Ptr = WriteTaggedData(&right_id, Buff_Ptr, 4);
	Buff_Ptr = WriteTaggedData(&deleted_length, Buff_Ptr, 5);

	Buff_Ptr = WriteSubblock(Buff_Ptr, 6, SubblockLen + sizeof(SubblockType));
	Buff_Ptr = Write(&SubblockType, Buff_Ptr);
	
	return Buff_Ptr;
}

size_t RMSceneItemBlock::SizeOfSceneItemDetails()
{
	return
		parent_id.SizeOf() + item_id.SizeOf() + left_id.SizeOf() + right_id.SizeOf() +
		5 * SIZE_OF_TAG + sizeof(UINT32) + SIZE_OF_SUBBLOCK + sizeof(UINT8);
}

