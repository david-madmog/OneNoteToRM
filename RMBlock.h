#pragma once
#include <exception>
#include <stdexcept>
#include <wtypes.h>

/*******************************************************************************

	RMBlock.h

	Header for generic ReMarkable file format blocks

	For a description on how it all works, please see RMBlockTypes.h

	(C) David Poirier 2026

	Based on knowledge from:
	* Rick Lupton - https://github.com/ricklupton rmscene
	* ddvk - https://github.com/ddvk 

*******************************************************************************/

enum BT_BlockType : INT8 {
	BT_UNKNOWN = -1,
	BT_MigrationInfo = 0,
	BT_SceneTree = 1,
	BT_TreeNode = 2,
	BT_SceneGlyphItem = 3,
	BT_SceneGroupItem = 4,
	BT_SceneLineItem = 5,
	BT_SceneTextItem = 6,
	BT_RootText = 7,
	BT_SceneTombstoneItem = 8,
	BT_AuthorIds = 9,
	BT_PageInfo = 10,
	BT_SceneInfo = 13
};


typedef struct rm_BlockHead {
	UINT32  len_body; //            doc : Byte count for block's main body.
	unsigned char magic1;
	unsigned char MinVersion;
	unsigned char CurrentVersion;
	BT_BlockType BlockType;
} RM_Blockhead;

struct incorrect_tag : public std::exception
{
	//using std::range_error::range_error;
	std::string s;
	incorrect_tag(std::string ss) : s(ss) {}
	~incorrect_tag() throw () {} // Updated
	const char* what() const throw() { return s.c_str(); }
};


#pragma pack( push, 1)

enum TagTypeEnum {
	//Tag type representing the type of following data."
	ID = 0x0F,
	Length4 = 0x0C,
	Byte8 = 0x08,
	Byte4 = 0x04,
	Byte1 = 0x01,
	UNKNOWN = 0
};

//enum BlockClass {
//	BCUnknown,
//	SceneTreeClass,
//	TreeNodeClass,
//	GroupItem,
//	Item,
//	RootTextClass
//};
//
typedef UINT8 RM_BOOL;
typedef char * RM_STRING;

typedef struct rm_Tag {
	int index = 0;
	TagTypeEnum TagType = UNKNOWN;
} RM_Tag;

typedef struct rm_CRDT_ID {
	UINT8 part1 = 0;
	int part2 = 0;

	friend bool operator< (const struct rm_CRDT_ID LHS, const struct rm_CRDT_ID RHS)
	{
		return std::tie(LHS.part1, LHS.part2) < std::tie(RHS.part1, RHS.part2);
	}
	friend bool operator== (const struct rm_CRDT_ID LHS, const struct rm_CRDT_ID RHS)
	{
		return std::tie(LHS.part1, LHS.part2) == std::tie(RHS.part1, RHS.part2);
	}
	friend std::wostream& operator<<(std::wostream& os, const rm_CRDT_ID RHS)
	{
		os << L"{" << RHS.part1 << L"," << RHS.part2 << L"}";
		return os;
	}
	friend std::ostream& operator<<(std::ostream& os, const rm_CRDT_ID RHS)
	{
		os << "{" << RHS.part1 << "," << RHS.part2 << "}";
		return os;
	}
public:
	size_t SizeOf() const {
		if (part2 < 0)
			return 6;
		if (part2 < 0x80)
			return 2;
		if (part2 < 0x4000)
			return 3;
		if (part2 < 0x200000)
			return 4;
		return 5;
	}
	size_t SizeOfT(int index = 0) const { return SizeOf() + (index > 7 ? 2 : 1); }
} RM_CRDT_ID;

//std::wostream& operator<< (std::wostream& stream, const RM_CRDT_ID& RHS);

static const size_t SIZE_OF_TAG = sizeof(UINT8);
static const size_t SIZE_OF_SUBBLOCK = sizeof(UINT32) + SIZE_OF_TAG;


template <typename T>
struct rm_CRDT_SEQ_ITEM {
	RM_CRDT_ID item_id = { 0, 0 };
	RM_CRDT_ID left_id = { 0, 0 };
	RM_CRDT_ID right_id = { 0, 0 };
	UINT32 deleted_length = 0;
	T value = 0;
	size_t SizeOfWithoutTemplateItem() { return SIZE_OF_SUBBLOCK + item_id.SizeOf() + left_id.SizeOf() + right_id.SizeOf() + sizeof(UINT32); }
	size_t SizeOfTWithoutTemplateItem() { return SIZE_OF_SUBBLOCK + item_id.SizeOfT() + left_id.SizeOfT() + right_id.SizeOfT() + sizeof(UINT32) + SIZE_OF_TAG; }
};

typedef struct rm_LWW_ID {
	RM_CRDT_ID timestamp = {0, 0};
	RM_CRDT_ID value = {0, 0};
	size_t SizeOf() const { return timestamp.SizeOf() + value.SizeOf() + 2 * SIZE_OF_TAG + SIZE_OF_SUBBLOCK; }
	size_t SizeOfT(int index = 0) const { return SizeOf() + (index > 7 ? 1 : 0); }
} RM_LWW_ID;

typedef struct rm_LWW_Bool {
	RM_CRDT_ID timestamp = { 0, 0 };
	RM_BOOL value = 0;
	size_t SizeOf() const { return timestamp.SizeOf() + sizeof(RM_BOOL) + 2 * SIZE_OF_TAG + SIZE_OF_SUBBLOCK; }
	size_t SizeOfT(int index = 0) const { return SizeOf() + (index > 7 ? 1 : 0); }
} RM_LWW_Bool;

typedef struct rm_LWW_String {
	RM_CRDT_ID timestamp = { 0, 0 };
	RM_STRING value = 0;
	size_t SizeOf() const
	{
		size_t string_length = (value ? strlen(value) : 0);
		if (string_length < 0x80)
			return timestamp.SizeOf() + 2 * SIZE_OF_SUBBLOCK + 1 + sizeof(RM_BOOL) + string_length;
		if (string_length < 0x4000)
			return timestamp.SizeOf() + 2 * SIZE_OF_SUBBLOCK + 2 + sizeof(RM_BOOL) + string_length;
		if (string_length < 0x200000)
			return timestamp.SizeOf() + 2 * SIZE_OF_SUBBLOCK + 3 + sizeof(RM_BOOL) + string_length;

		return timestamp.SizeOf() + 2 * SIZE_OF_SUBBLOCK + 4 + sizeof(RM_BOOL) + string_length;
	}
	size_t SizeOfT(int index = 0) const { return SizeOf() + (index > 7 ? 2 : 1); }
} RM_LWW_String;

typedef struct rm_LWW_Byte {
	RM_CRDT_ID timestamp = { 0, 0 };
	UINT8 value = 0;
	size_t SizeOf() const { return timestamp.SizeOf() + sizeof(UINT8) + 2 * SIZE_OF_TAG + SIZE_OF_SUBBLOCK; }
	size_t SizeOfT(int index = 0) const { return SizeOf() + (index > 7 ? 1 : 0); }
} RM_LWW_Byte;

typedef struct rm_LWW_Float {
	RM_CRDT_ID timestamp = { 0, 0 };
	FLOAT value = 0;
	size_t SizeOf() const { return timestamp.SizeOf() + sizeof(FLOAT) + 2 * SIZE_OF_TAG + SIZE_OF_SUBBLOCK; }
	size_t SizeOfT(int index = 0) const { return SizeOf() + (index > 7 ? 1 : 0); }
} RM_LWW_Float;

#pragma pack( pop )

class RMBlock {
protected:
	static void* ReadTaggedData(RM_LWW_ID * ID, void* Buff_Ptr, int index);
	static void* ReadTaggedData(RM_LWW_Bool* ID, void* Buff_Ptr, int index);
	static void* ReadTaggedData(RM_LWW_Byte* ID, void* Buff_Ptr, int index);
	static void* ReadTaggedData(RM_LWW_String* ID, void* Buff_Ptr, int index);
	static void* ReadTaggedData(RM_LWW_Float* ID, void* Buff_Ptr, int index);
	static void* ReadTaggedData(RM_CRDT_ID* ID, void* Buff_Ptr, int index);
	static void* ReadTaggedData(RM_BOOL* Bool, void* Buff_Ptr, int index);
	static void* ReadTaggedData(UINT32* data, void* Buff_Ptr, int index);
	static void* ReadTaggedData(FLOAT* data, void* Buff_Ptr, int index);
	static void* ReadTaggedData(DOUBLE* data, void* Buff_Ptr, int index);

	static void* WriteTaggedData(RM_CRDT_ID* ID, void* Buff_Ptr, int index);
	static void* WriteTaggedData(RM_BOOL* Bool, void* Buff_Ptr, int index);
	static void* WriteTaggedData(RM_LWW_ID* ID, void* Buff_Ptr, int index);
	static void* WriteTaggedData(RM_LWW_Bool* data, void* Buff_Ptr, int index);
	static void* WriteTaggedData(RM_LWW_Byte* data, void* Buff_Ptr, int index);
	static void* WriteTaggedData(RM_LWW_String* data, void* Buff_Ptr, int index);
	static void* WriteTaggedData(RM_LWW_Float* data, void* Buff_Ptr, int index);
	static void* WriteTaggedData(UINT32* data, void* Buff_Ptr, int index);
	static void* WriteTaggedData(FLOAT* data, void* Buff_Ptr, int index);
	static void* WriteTaggedData(DOUBLE* data, void* Buff_Ptr, int index);

	static void* ReadTaggedDataOptional(UINT32* data, void* Buff_Ptr, int index);
	static void* ReadTaggedDataOptional(UINT8* data, void* Buff_Ptr, int index);
	static void* ReadTaggedDataOptional(RM_CRDT_ID* data, void* Buff_Ptr, int index);

	static void* ReadIntPair(UINT32* Ints, void* Buff_Ptr, int index);
	static void* ReadVarUINT(int * data, void* Buff_Ptr);
	static void* ReadTag(RM_Tag* tag, void* Buff_Ptr, int index, TagTypeEnum TagType);
	static void* ReadString(RM_STRING * data, void* Buff_Ptr, int index);
	static void* ReadStringWithFormat(RM_STRING* data, UINT32* fmt, void* Buff_Ptr, int index);
	static void* ReadSubblock(void* Buff_Ptr, int index);
	static void* ReadSubblock(void* Buff_Ptr, int index, UINT32* subblock_length);

	static void* WriteIntPair(UINT32* Ints, void* Buff_Ptr, int index);
	static void* WriteVarUINT(int data, void* Buff_Ptr);
	static size_t VarUINTLen(int data);
//	static size_t RM_CRDT_Len(const rm_CRDT_ID& ID);

	static void* WriteTag(void* Buff_Ptr, int index, TagTypeEnum TagType);
	static void* WriteString(RM_STRING* data, void* Buff_Ptr, int index);
	static size_t StringWriteSize(RM_STRING data);
	static void* WriteStringWithFormat(RM_STRING* data, UINT32* fmt, void* Buff_Ptr, int index);
	static void* WriteSubblock(void* Buff_Ptr, int index, size_t subblock_length);

	static void* Read(UINT8* data, void* Buff_Ptr, int count = 1);
	static void* Read(UINT16* data, void* Buff_Ptr, int count = 1);
	static void* Read(FLOAT* data, void* Buff_Ptr, int count = 1);
	static void* Read(DOUBLE* data, void* Buff_Ptr, int count = 1);
	static void* Read(RM_CRDT_ID* ID, void* Buff_Ptr, int count = 1);

	static void* ReadTextItem(struct rm_CRDT_SEQ_ITEM<RM_STRING>* data, void* Buff_Ptr);

	static void* Write(UINT8* data, void* Buff_Ptr, int count = 1);
	static void* Write(UINT16* data, void* Buff_Ptr, int count = 1);
	static void* Write(UINT32* data, void* Buff_Ptr, int count = 1);
	static void* Write(FLOAT* data, void* Buff_Ptr, int count = 1);
	static void* Write(DOUBLE* data, void* Buff_Ptr, int count = 1);
	static void* Write(RM_CRDT_ID* ID, void* Buff_Ptr, int count = 1);

	static void* WriteTextItem(rm_CRDT_SEQ_ITEM<RM_STRING>* data, void* Buff_Ptr);

	void* WriteBuff = nullptr;
	size_t WriteBuffLen = 0;

	void* WriteBlockHead(void* Buff_Ptr, size_t block_length, unsigned char version = 1);

public:
	RMBlock();
	virtual ~RMBlock() {};
	virtual bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version) { return false; };
	inline virtual BT_BlockType BlockType() { return BT_UNKNOWN; };

	virtual size_t PrepareWrite();
	void* WriteBlock(void* Buff);
};

enum BT_SubBlockType : UINT8 {
	SBT_UNKNOWN = 0,
	SBT_GlyphBlock = 1,
	SBT_GroupBlock = 2,
	SBT_LineBlock = 3,
	SBT_TextBlock = 4
};

class RMSceneItemBlock : public RMBlock {
protected:

public:
	RMSceneItemBlock();
	bool ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version = 2) { return false; } ;
	void* ReadSceneItemDetails(const unsigned char* Buff, size_t ValidLen);
	void* WriteSceneItemDetails(void* Buff_Ptr, size_t SubblockLen);
	size_t SizeOfSceneItemDetails();
	inline virtual BT_SubBlockType SubBlockType() { return SBT_UNKNOWN; };

	RM_CRDT_ID parent_id;
	RM_CRDT_ID item_id;
	RM_CRDT_ID left_id;
	RM_CRDT_ID right_id;
	UINT32 deleted_length = 0;
};