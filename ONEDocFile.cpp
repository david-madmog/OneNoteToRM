#include "ONEDocFile.h"

// We need to include possible types so compiler knows what instantiations we need
#include "WindowONEPage.h"
template int ONEDocFile<WindowONEPage>::ExtractFromONE(const char* FileName);
template void ONEDocFile<WindowONEPage>::DrawPage(void* DrawDetails, int PageID);
//template ONEDocFile<WindowONEPage>::~ONEDocFile();

//template<class PageType> ONEDocFile<PageType>::~ONEDocFile()
//{
//	for (auto Buff : Buffers) {
//		free(Buff);
//	}
//}

template<class PageType> int ONEDocFile<PageType>::ExtractFromONE(const char* FileName)
{
	//The header(section 2.3.1) is the first 1024 bytes of the file.It contains references to the other structures in the file as well as metadata about the file.
	//	The free chunk list(section 2.3.2) defines where there are free spaces in the file where data can be written.
	//	The transaction log(section 2.3.3) stores the state and length of each file node list(section 2.4) in the file.
	//	The hashed chunk list(section 2.3.4) stores read - only objects in the file that can be referenced by multiple revisions(section 2.1.8).
	//	The root file node list(section 2.1.14) is the file node list that is the root of the tree of all file node lists in the file.
	//	All of the file node lists that contain user data.
//	void* Buff_Ptr;
	ONE_Header Header{};

	std::ifstream ONEFile(FileName, std::ios::binary);

	// So, in read mode, first we get the file header
	ONEFile.read((char*)&Header, sizeof(Header));
	UUID FileType;
	if ( UuidFromStringA((RPC_CSTR)"7B5C52E4-D88C-4DA7-AEB1-5378D02996D3", &FileType) != RPC_S_OK)
		return 0; 
	assert(Header.FileType == FileType);

	// Now, the header tells us where the root file node list is, so lets get that.
	OneDocFileChunk RNFL;
	RNFL.LoadFileNodeListFragment(&ONEFile, Header.FileNodeListRoot);

	// Now, we look at the root FileNode list which tells us where the root object space is
	//		We can use the ObjectSpaceManifestRootFND to find the extended GUID of the root object space
	//		We can then look at the various ObjectSpaceManifestListReferenceFND's to find the reference of that object space
	ONE_FileNode* ObjectSpaceManifestRoot = RNFL.LocateFileNodeInList(ObjectSpaceManifestRootFND);
	ONE_FileNode* ObjectSpaceManifestRootRef = RNFL.LocateFileNodeInList(ObjectSpaceManifestListReferenceFND, ObjectSpaceManifestRoot->Ext_Guid);

	// Next, the Object space will contain an Object Space Manifest List Start (which does nothing but confirm we're in the right place really)
	// and a number of Revision manifest list references
	OneDocFileChunk OSML;
	OSML.LoadFileNodeListFragment(&ONEFile, ObjectSpaceManifestRootRef->ChunkRef);
	ONE_FileNode* ObjectSpaceManifestListStart = OSML.LocateFileNodeInList(ObjectSpaceManifestListStartFND);
	assert(ObjectSpaceManifestListStart->Ext_Guid == ObjectSpaceManifestRoot->Ext_Guid);
	ONE_FileNode* RevisionManifestListReference = OSML.LocateFileNodeInList(RevisionManifestListReferenceFND);

	// Next, the revision manifest list consisists of a list start, then a set of Revision manifest start (one of three flavours), an object group
	// list referencence and manifest end. The Object Group List Reference directs us to the object group list
	OneDocFileChunk RML;
	RML.LoadFileNodeListFragment(&ONEFile, RevisionManifestListReference->ChunkRef);
	ONE_FileNode* ObjectGroupListReference = RML.LocateFileNodeInList(ObjectGroupListReferenceFND);

	// The Object group list consists of a start, global ID table start, entry (one of three flavours) and end, and then data signature group 
	// definition () and Object declaration 2 ref count - which directs us to the object space object prop sets which represent the actual doc contents
	// Object group start is just identifying ID
	OneDocFileChunk OGL;
	void * OGLBuffer = OGL.LoadFileNodeListFragment(&ONEFile, ObjectGroupListReference->ChunkRef);

	std::map<UINT32, GUID> GlobalIndexTable;
	std::vector<OneDocObject*>DocObjects;
	ONE_FileNode FileNode;
	bool bDone = false;
	void* Local_Buff = OGLBuffer;
	while (!bDone)
	{
		Local_Buff = OGL.ParseFileNode(&FileNode, Local_Buff);
		if (FileNode.FileNodeID == ObjectDeclaration2RefCountFND) {
			OneDocObject* DocObject = new OneDocObject(FileNode.ObjectDeclarationBody.jcid);
			DocObject->LoadFileNodeListFragment(&ONEFile, FileNode.ChunkRef);
			DocObjects.push_back(DocObject);
		}

		if (FileNode.FileNodeID == GlobalIdTableEntryFNDX)
		{
			GlobalIndexTable[FileNode.GlobalIDTableIndex] = FileNode.Ext_Guid.Guid;
		}

		if (FileNode.FileNodeID == 0x0FF ||
			FileNode.FileNodeID == ObjectGroupEndFND ||
			Local_Buff >= (void *)((char *)OGLBuffer + ObjectGroupListReference->ChunkRef.cb))
			bDone = true;
	}

	sprintf_s(LogBuffer, LB_SIZE, "Found %zd Object Declarations", DocObjects.size());
	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);
	
	// Now, process each of the objects we found
	for (auto DocObject : DocObjects) {
		DocObject->ParseObjectPropSet();

		if (DocObject->PageObjectSpace.Index || DocObject->PageObjectSpace.n )
		{
			// Aha! This Doc object can tell us the GUID of the page space
			ONE_Ext_GUID PageSpaceGuid;
			PageSpaceGuid.n = DocObject->PageObjectSpace.n;
			PageSpaceGuid.Guid = GlobalIndexTable[DocObject->PageObjectSpace.Index];
			ONE_FileNode* PageSpaceManifestList = RNFL.LocateFileNodeInList(ObjectSpaceManifestListReferenceFND, PageSpaceGuid);
			if (PageSpaceManifestList) 
			{
				PageType* Page = new PageType();
				Page->LoadPage(&ONEFile, PageSpaceManifestList);
				Pages.push_back(Page);
			}
		}
	}

	ONEFile.close();

	return 0;
}

template<class PageType> void ONEDocFile<PageType>::DrawPage(void* DrawDetails, int PageID)
{
	for (auto Page : Pages)
	{
		Page->DrawPage(DrawDetails);
	}
}