
#include "OneDocFileChunk.h"

void* OneDocFileChunk::LoadFileNodeListFragment(std::istream* ONEFile, ONE_FileChunkReference ChunkRef) {
	// Memory management
	if (ReadBuffer)
		free((void *)ReadBuffer);
	// Now, the header tells us where the root file node list is, so lets get that.

	ReadBuffer = (char*)malloc(ChunkRef.cb);
	ONEFile->seekg(ChunkRef.stp);
	ONEFile->read(ReadBuffer, ChunkRef.cb);
	ONE_FileNodeListHeader* RNFLHeader = (ONE_FileNodeListHeader*)ReadBuffer;
	assert(RNFLHeader);
	if (Magic)
		assert(RNFLHeader->Magic == Magic);

	return ReadBuffer + sizeof(ONE_FileNodeListHeader);
}

void* OneDocFileChunk::LoadFileNodeListFragment(std::istream* ONEFile, ONE_FileChunkReference64x32 ChunkRef)
{
	ONE_FileChunkReference FCR = { ChunkRef.stp, ChunkRef.cb };
	return LoadFileNodeListFragment(ONEFile, FCR);
}

OneDocFileChunk::~OneDocFileChunk() {
	if (ReadBuffer)
		free((void*)ReadBuffer);
}

void* OneDocFileChunk::ParseFileNode(ONE_FileNode* FileNode, void* Buff_Ptr) {
	//UINT32 Tmp;
	void* Local_Ptr;

	*((UINT32*)FileNode) = *(UINT32*)Buff_Ptr;
	Local_Ptr = (void*)((UINT32*)Buff_Ptr + 1);

	if (FileNode->C != 0)
	{
		switch (FileNode->A) {
		case 0:
			FileNode->ChunkRef.stp = *(UINT64*)Local_Ptr;
			Local_Ptr = (void*)((UINT64*)Local_Ptr + 1);
			break;
		case 1:
			FileNode->ChunkRef.stp = *(UINT32*)Local_Ptr;
			Local_Ptr = (void*)((UINT32*)Local_Ptr + 1);
			break;
		case 2:
			FileNode->ChunkRef.stp = (*(UINT16*)Local_Ptr) * 8;
			Local_Ptr = (void*)((UINT16*)Local_Ptr + 1);
			break;
		case 3:
			FileNode->ChunkRef.stp = (*(UINT32*)Local_Ptr) * 8;
			Local_Ptr = (void*)((UINT32*)Local_Ptr + 1);
			break;
		}

		switch (FileNode->B) {
		case 0:
			FileNode->ChunkRef.cb = *(UINT32*)Local_Ptr;
			Local_Ptr = (void*)((UINT32*)Local_Ptr + 1);
			break;
		case 1:
			FileNode->ChunkRef.cb = *(UINT64*)Local_Ptr;
			Local_Ptr = (void*)((UINT64*)Local_Ptr + 1);
			break;
		case 2:
			FileNode->ChunkRef.cb = (*(UINT8*)Local_Ptr) * 8;
			Local_Ptr = (void*)((UINT8*)Local_Ptr + 1);
			break;
		case 3:
			FileNode->ChunkRef.cb = (*(UINT16*)Local_Ptr) * 8;
			Local_Ptr = (void*)((UINT16*)Local_Ptr + 1);
			break;
		}
	}
	if (FileNode->FileNodeID == ObjectSpaceManifestRootFND ||
		FileNode->FileNodeID == ObjectSpaceManifestListReferenceFND ||
		FileNode->FileNodeID == ObjectSpaceManifestListStartFND ||
		FileNode->FileNodeID == RevisionManifestListReferenceFND ||
		FileNode->FileNodeID == ObjectGroupListReferenceFND
		)
	{
		// These items have an Ext ID next
		memcpy(&FileNode->Ext_Guid, Local_Ptr, sizeof(ONE_Ext_GUID));
		Local_Ptr = (void*)((UINT8*)Local_Ptr + sizeof(ONE_Ext_GUID));
	}

	if (FileNode->FileNodeID == ObjectDeclaration2RefCountFND) {
		memcpy(&FileNode->ObjectDeclarationBody, Local_Ptr, sizeof(ONE_ObjectDeclaration2Body));
		Local_Ptr = (void*)((UINT8*)Local_Ptr + sizeof(ONE_ObjectDeclaration2Body));
	}

	if (FileNode->FileNodeID == GlobalIdTableEntryFNDX) {
		FileNode->GlobalIDTableIndex = *((UINT32 *)Local_Ptr);
		Local_Ptr = (void*)((UINT32*)Local_Ptr + 1);
		memcpy(&FileNode->Ext_Guid.Guid, Local_Ptr, sizeof(GUID));
		Local_Ptr = (void*)((UINT8*)Local_Ptr + sizeof(GUID));

	}

	Buff_Ptr = (void*)((UINT8*)Buff_Ptr + FileNode->Size);
	return Buff_Ptr;
}

ONE_FileNode* OneDocFileChunk::LocateFileNodeInList(one_FileNodeType NodeType, void* Buff_Ptr, ONE_Ext_GUID ExtGuid) {
	ONE_FileNode* FileNode = new ONE_FileNode;

	bool bDone = false;
	while (!bDone)
	{
		Buff_Ptr = ParseFileNode(FileNode, Buff_Ptr);
		if (FileNode->FileNodeID == NodeType && (FileNode->Ext_Guid == ExtGuid || ExtGuid == NullGuid))
			return FileNode;

		if (FileNode->FileNodeID == 0x0FF ||
			FileNode->FileNodeID == 0 )
			bDone = true;
	}

	return nullptr;
}

