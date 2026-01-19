#pragma once
#include <exception>
#include <stdexcept>
#include <wtypes.h>
#include "DOCXToRM.h"


struct incorrect_tag : std::range_error
{
	using std::range_error::range_error;
};


#pragma pack(1)

enum TagTypeEnum {
	//Tag type representing the type of following data."
	ID = 0x0F,
	Length4 = 0x0C,
	Byte8 = 0x08,
	Byte4 = 0x04,
	Byte1 = 0x01,
	UNKNOWN = 0
};

enum BlockClass {
	BCUnknown,
	SceneTreeClass,
	TreeNodeClass,
	GroupItem,
	Item,
	RootTextClass
};

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
} RM_CRDT_ID;

typedef struct rm_LWW_ID {
	RM_CRDT_ID timestamp = {0, 0};
	RM_CRDT_ID value = {0, 0};
} RM_LWW_ID;

typedef struct rm_LWW_Bool {
	RM_CRDT_ID timestamp = { 0, 0 };
	RM_BOOL value = 0;
} RM_LWW_Bool;

typedef struct rm_LWW_String {
	RM_CRDT_ID timestamp = { 0, 0 };
	RM_STRING value = 0;
} RM_LWW_String;

typedef struct rm_LWW_Byte {
	RM_CRDT_ID timestamp = { 0, 0 };
	UINT8 value = 0;
} RM_LWW_Byte;

typedef struct rm_LWW_Float {
	RM_CRDT_ID timestamp = { 0, 0 };
	FLOAT value = 0;
} RM_LWW_Float;

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

	static void* ReadTaggedDataOptional(UINT32* data, void* Buff_Ptr, int index);

	static void* ReadIntPair(UINT32* Ints, void* Buff_Ptr, int index);
	static void* ReadVarUINT(int * data, void* Buff_Ptr);
	static void* ReadTag(RM_Tag* tag, void* Buff_Ptr, int index, TagTypeEnum TagType);
	static void* ReadString(RM_STRING * data, void* Buff_Ptr, int index);
	static void* ReadSubblock(void* Buff_Ptr, int index);
	static void* ReadSubblock(void* Buff_Ptr, int index, UINT32* subblock_length);

	static void* Read(UINT8* data, void* Buff_Ptr, int count = 1);
	static void* Read(UINT16* data, void* Buff_Ptr, int count = 1);
	static void* Read(FLOAT* data, void* Buff_Ptr, int count = 1);

public:
	RMBlock();
	virtual void ParseBuffer(const unsigned char * Buff, size_t ValidLen, int version = 2) = 0;
	BlockClass Class;
};


class RMSceneItemBlock : public RMBlock {
protected:

public:
	RMSceneItemBlock();
	void ParseBuffer(const unsigned char* Buff, size_t ValidLen, int version = 2) {} ;
	void* ReadSceneItemDetails(const unsigned char* Buff, size_t ValidLen);

	RM_CRDT_ID parent_id;
	RM_CRDT_ID item_id;
	RM_CRDT_ID left_id;
	RM_CRDT_ID right_id;
	UINT32 deleted_length = 0;
};