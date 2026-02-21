#pragma once
#include "framework.h"

#pragma pack(1)


typedef struct one_FileChunkReference32
{
	UINT32 stp; //An unsigned integer that specifies the location of the referenced data in the file.
	UINT32 cb;  //An unsigned integer that specifies the size, in bytes, of the referenced data.
} ONE_FileChunkReference32;

typedef struct one_FileChunkReference64x32
{
	UINT64 stp; //An unsigned integer that specifies the location of the referenced data in the file.
	UINT32 cb;  //An unsigned integer that specifies the size, in bytes, of the referenced data.
} ONE_FileChunkReference64x32;

typedef struct one_FileChunkReference
{
	UINT64 stp; //An unsigned integer that specifies the location of the referenced data in the file.
	UINT64 cb;  //An unsigned integer that specifies the size, in bytes, of the referenced data.
} ONE_FileChunkReference;

typedef struct one_Header
{
	GUID FileType;          // {7B5C52E4-D88C-4DA7-AEB1-5378D02996D3} for a .one file
	GUID File;
	GUID LegacyFileVersion; // MUST be "{00000000-0000-0000-0000-000000000000}" and MUST be ignored
	GUID FileFormat;        //MUST be "{109ADD3F-911B-49F5-A5D0-1791EDC8AED8}"
	UINT32 LastCodeThatWroteToThisFile;             // 	0x0000002A for a .one file
	UINT32 OldestCodeThatHasWrittenToThisFile;
	UINT32 NewestCodeThatHasWrittenToThisFile;
	UINT32 OldestCodeThatMayReadThisFile;
	ONE_FileChunkReference32 LegacyFreeChunkList;  // MUST have a value of "fcrZero"
	ONE_FileChunkReference32 LegacyTransactionLog; // must be fcrNil: all bits of the stp field are set to 1, and all bits of the cb field are set to zero.
	UINT32 TransactionsInLog; // specifies the number of transactions in the transaction log
	UINT32 LegacyExpectedFileLength; //MUST be zero, and MUST be ignored
	UINT64 rgbPlaceholder; //  MUST be zero, and MUST be ignored.
	ONE_FileChunkReference32 LegacyFileNodeListRoot; //MUST be "fcrNil".
	UINT32 LegacyFreeSpaceInFreeChunkList; //MUST be zero, and MUST be ignored.
	UINT8 NeedsDefrag; //MUST be ignored.
	UINT8 RepairedFile; //MUST be ignored.
	UINT8 NeedsGarbageCollect; //MUST be ignored.
	UINT8 HasNoEmbeddedFileObjects; //An unsigned integer that MUST be zero, and MUST be ignored.
	GUID Ancestor; //A GUID that specifies the Header.guidFile field of the table of contents file. If the GUID is {00000000-0000-0000-0000-000000000000}, this field does not reference a table of contents file.
	UINT32 crcName; // specifies the CRC value(section 2.1.2) of the name of this revision store file.The name is the Unicode representation of the file name with its extension and an additional null character at the end.This CRC is calculated using the CRC algorithm for the.one file(section 2.1.2), regardless of this revision store file format.
	ONE_FileChunkReference64x32 HashedChunkList; // specifies a reference to the first FileNodeListFragment in a hashed chunk list(section 2.3.4).If the value of the FileChunkReference64x32 structure is "fcrZero" or "fcrNil", the hashed chunk list does not exist.
	ONE_FileChunkReference64x32	TransactionLog; //specifies a reference to the first TransactionLogFragment structure(section 2.3.3.1) in a transaction log(section 2.3.3).The value of the fcrTransactionLog field MUST NOT be "fcrZero" and MUST NOT be "fcrNil".
	ONE_FileChunkReference64x32 FileNodeListRoot; //specifies a reference to a root file node list(section 2.1.14).The value of the fcrFileNodeListRoot field MUST NOT be "fcrZero" and MUST NOT be "fcrNil".
	ONE_FileChunkReference64x32 FreeChunkList; //specifies a reference to the first FreeChunkListFragment structure(section 2.3.2.1).If the value of the FileChunkReference64x32 structure is "fcrZero" or "fcrNil", then the free chunk list(section 2.3.2) does not exist.
	UINT64	ExpectedFileLength; //specifies the size, in bytes, of this revision store file.
	UINT64	FreeSpaceInFreeChunkList; // SHOULD<3> specify the size, in bytes, of the free space specified by the free chunk list.
	GUID	FileVersion; //When either the value of cTransactionsInLog field or the guidDenyReadFileVersion field is being changed, guidFileVersion MUST be changed to a new GUID.
	UINT64	FileVersionGeneration; //specifies the number of times the file has changed.MUST be incremented when the guidFileVersion field changes.
	GUID	DenyReadFileVersion; //When the existing contents of the file are being changed, excluding the Header structure of the file and unused storage blocks, guidDenyReadFileVersion MUST be changed to a new GUID.
	UINT32	DebugLogFlags; //MUST be zero.MUST be ignored.
	ONE_FileChunkReference64x32 DebugLog; //MUST have a value "fcrZero".MUST be ignored.
	ONE_FileChunkReference64x32	AllocVerificationFreeChunkList; //MUST be "fcrZero".MUST be ignored.
	UINT32	Created; //specifies the build number of the application that created this revision store file.SHOULD<4> be ignored.
	UINT32	LastWroteToThisFile; //specifies the build number of the application that last wrote to this revision store file.SHOULD<5> be ignored.
	UINT32	OldestWritten; //specifies the build number of the oldest application that wrote to this revision store file.SHOULD<6> be ignored.
	UINT32		NewestWritten; //specifies the build number of the newest application that wrote to this revision store file.SHOULD<7> be ignored.
	UINT8	rgbReserved[728]; //MUST be zero.MUST be ignored.
} ONE_Header;

typedef struct one_FileNodeListHeader {
	UINT64 Magic; //MUST be "0xA4567AB1F5F7F4C4".
	UINT32 FileNodeListID; // specifies the identity of the file node list(section 2.4) this fragment belongs to.MUST be equal to or greater than 0x00000010. The pair of FileNodeListID and nFragmentSequence fields MUST be unique relative to other FileNodeListFragment structures in the file.
	UINT32 FragmentSequence; // specifies the index of the fragment in the file node list containing the fragment.The nFragmentSequence field of the first fragment in a given file node list MUST be 0 and the nFragmentSequence fields of all subsequent fragments in this list MUST be sequential.
} ONE_FileNodeListHeader;

enum one_FileNodeType {
	ObjectSpaceManifestRootFND = 0x004,
	ObjectSpaceManifestListReferenceFND = 0x008,
	ObjectSpaceManifestListStartFND = 0x00C,
	RevisionManifestListReferenceFND = 0x010,
	RevisionManifestListStartFND = 0x014,
	RevisionManifestStart4FND = 0x01B,
	RevisionManifestEndFND = 0x01C,
	RevisionManifestStart6FND = 0x01E,
	RevisionManifestStart7FND = 0x01F,
	GlobalIdTableStartFNDX = 0x021,
	GlobalIdTableStart2FND = 0x022,
	GlobalIdTableEntryFNDX = 0x024,
	GlobalIdTableEntry2FNDX = 0x025,
	GlobalIdTableEntry3FNDX = 0x026,
	GlobalIdTableEndFNDX = 0x028,
	ObjectDeclarationWithRefCountFNDX = 0x02D,
	ObjectDeclarationWithRefCount2FNDX = 0x02E,
	ObjectRevisionWithRefCountFNDX = 0x041,
	ObjectRevisionWithRefCount2FNDX = 0x042,
	RootObjectReference2FNDX = 0x059,
	RootObjectReference3FND = 0x05A,
	RevisionRoleDeclarationFND = 0x05C,
	RevisionRoleAndContextDeclarationFND = 0x05D,
	ObjectDeclarationFileData3RefCountFND = 0x072,
	ObjectDeclarationFileData3LargeRefCountFND = 0x073,
	ObjectDataEncryptionKeyV2FNDX = 0x07C,
	ObjectInfoDependencyOverridesFND = 0x084,
	DataSignatureGroupDefinitionFND = 0x08C,
	FileDataStoreListReferenceFND = 0x090,
	FileDataStoreObjectReferenceFND = 0x094,
	ObjectDeclaration2RefCountFND = 0x0A4,
	ObjectDeclaration2LargeRefCountFND = 0x0A5,
	ObjectGroupListReferenceFND = 0x0B0,
	ObjectGroupStartFND = 0x0B4,
	ObjectGroupEndFND = 0x0B8,
	HashedChunkDescriptor2FND = 0x0C2,
	ReadOnlyObjectDeclaration2RefCountFND = 0x0C4,
	ReadOnlyObjectDeclaration2LargeRefCountFND = 0x0C5,
	ChunkTerminatorFND = 0x0FF
};


typedef struct one_Ext_GUID {
	GUID Guid;
	UINT32 n;
	friend bool operator== (const struct one_Ext_GUID LHS, const struct one_Ext_GUID RHS)
	{
		return (LHS.Guid == RHS.Guid);
	}
} ONE_Ext_GUID;

constexpr ONE_Ext_GUID NullGuid = { 0, 0 };


enum JCID : UINT32 {
	jcidNULL = 0,
	jcidReadOnlyPersistablePropertyContainerForAuthor = 0x00120001,
	jcidPersistablePropertyContainerForTOC = 0x00020001,
	jcidPersistablePropertyContainerForTOCSection = 0x00020001,
	jcidSectionNode = 0x00060007,
	jcidPageSeriesNode = 0x00060008,
	jcidPageNode = 0x0006000B,
	jcidOutlineNode = 0x0006000C,
	jcidOutlineElementNode = 0x0006000D,
	jcidRichTextOENode = 0x0006000E,
	jcidImageNode = 0x00060011,
	jcidNumberListNode = 0x00060012,
	jcidOutlineGroup = 0x00060019,
	jcidTableNode = 0x00060022,
	jcidTableRowNode = 0x00060023,
	jcidTableCellNode = 0x00060024,
	jcidTitleNode = 0x0006002C,
	jcidPageMetaData = 0x00020030,
	jcidSectionMetaData = 0x00020031,
	jcidEmbeddedFileNode = 0x00060035,
	jcidPageManifestNode = 0x00060037,
	jcidConflictPageMetaData = 0x00020038,
	jcidVersionHistoryContent = 0x0006003C,
	jcidVersionProxy = 0x0006003D,
	jcidNoteTagSharedDefinitionContainer = 0x00120043,
	jcidRevisionMetaData = 0x00020044,
	jcidVersionHistoryMetaData = 0x00020046,
	jcidParagraphStyleObject = 0x0012004D,
	jcidParagraphStyleObjectForText = 0x0012004D
};

typedef struct one_ObjectDeclaration2Body
{
	UINT32 oid;
	JCID jcid;
	unsigned int A : 1;
	unsigned int B : 1;
} ONE_ObjectDeclaration2Body;

typedef struct one_FileNode {
	one_FileNodeType FileNodeID : 10;
	unsigned int Size : 13;
	unsigned int A : 2;
	unsigned int B : 2;
	unsigned int C : 4;
	unsigned int D : 1;
	UINT32 GlobalIDTableIndex;
	ONE_Ext_GUID Ext_Guid;
	ONE_FileChunkReference ChunkRef;
	ONE_ObjectDeclaration2Body ObjectDeclarationBody;
} ONE_FileNode;


class OneDocFileChunk 
{
private:

protected:
	UINT64 Magic;
	char* ReadBuffer = nullptr;

public:
	OneDocFileChunk() { Magic = 0xA4567AB1F5F7F4C4; }
		
	void* ParseFileNode(ONE_FileNode* FileNode, void* Buff_Ptr);
	void* LoadFileNodeListFragment(std::istream* ONEFile, ONE_FileChunkReference ChunkRef);
	void* LoadFileNodeListFragment(std::istream* ONEFile, ONE_FileChunkReference64x32 ChunkRef);
	ONE_FileNode* LocateFileNodeInList(one_FileNodeType NodeType, void* Buff_Ptr, ONE_Ext_GUID ExtGuid );
	ONE_FileNode* LocateFileNodeInList(one_FileNodeType NodeType, ONE_Ext_GUID ExtGuid) { return LocateFileNodeInList(NodeType, (char*)ReadBuffer + sizeof(ONE_FileNodeListHeader), ExtGuid); }
	ONE_FileNode* LocateFileNodeInList(one_FileNodeType NodeType) {	return LocateFileNodeInList(NodeType, (char*)ReadBuffer + sizeof(ONE_FileNodeListHeader), NullGuid);}

	~OneDocFileChunk();
};

