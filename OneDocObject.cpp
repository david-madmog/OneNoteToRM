
#include "OneDocObject.h"
#include "DOCXToRM.h"

//OneDocObject::OneDocObject(ONE_FileNode* FileNode) {
//	sprintf_s(LogBuffer, LB_SIZE, "Creating Doc Object: JCID = %X ", FileNode->ObjectDeclarationBody.jcid);
//	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);
//
//	ChunkRef = FileNode->ChunkRef;
//	// This chunk ref is a ref to an ObjectSpaceObjectPropSet:
//	
//	
//	ObjectDeclarationBody = FileNode->ObjectDeclarationBody;
//
//}



void* OneDocObject::ParseObjectPropSet() {
	
	// First, some OID's
	ONE_ObjectStreamHeader* header = (ONE_ObjectStreamHeader*)ReadBuffer;
	UINT8* Local_Ptr = (UINT8*)ReadBuffer + sizeof(ONE_ObjectStreamHeader);

	// Just stash them for now
	for (unsigned int i = 0; i < header->Count; i++) {
		ObjectIDs.push_back(*((UINT32*)Local_Ptr));
		Local_Ptr += sizeof(UINT32);
	}

	// Now some OSID's
	if (header->OsidStreamNotPresent == false)
	{
		ONE_ObjectStreamHeader* OSIDSheader = (ONE_ObjectStreamHeader*)Local_Ptr;
		Local_Ptr += sizeof(ONE_ObjectStreamHeader);
		
		// Just stash them for now
		for (unsigned int i = 0; i < OSIDSheader->Count; i++) {
			ObjectSpaceIDs.push_back(*((ONE_compactID*)Local_Ptr));
			Local_Ptr += sizeof(ONE_compactID);
		}

		// Now some ContextID's
		if (OSIDSheader->ExtendedStreamsPresent) {
			ONE_ObjectStreamHeader* ContextIDSheader = (ONE_ObjectStreamHeader*)Local_Ptr;
			Local_Ptr += sizeof(ONE_ObjectStreamHeader);

			// Just stash them for now
			for (unsigned int i = 0; i < ContextIDSheader->Count; i++) {
				ContextIDs.push_back(*((UINT32*)Local_Ptr));
				Local_Ptr += sizeof(UINT32);
			}

		}
	}

	// Now we get to the body: A property Set structure
	UINT16 NumProperties = *Local_Ptr;
	Local_Ptr += sizeof(UINT16);

	sprintf_s(LogBuffer, LB_SIZE, "Object JCID = %X. Found %d Properties: ", jcid, NumProperties);
	DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);



	std::vector<ONE_PropertyID> propertyIDs;
	for (int i = 0; i < NumProperties; i++) {
		ONE_PropertyID PID = *(ONE_PropertyID*)Local_Ptr;
		Local_Ptr += sizeof(ONE_PropertyID);
		propertyIDs.push_back(PID);
	}

	UINT32 Len;
	ONE_Property Prop;
	for (auto PID: propertyIDs)
	{
		Prop.id = PID.id;
		switch (PID.type) {
		case 0x01:
			sprintf_s(LogBuffer, LB_SIZE, "...Property has no data ID:%x", PID.id);
			break;
		case 0x02:
			Prop.Data = PID.boolValue;
			sprintf_s(LogBuffer, LB_SIZE, "...Property is BOOL data ID:%X (%d)", PID.id, PID.boolValue);
			break;
		case 0x03:
			Prop.Data = *(UINT8*)Local_Ptr;
			Local_Ptr += sizeof(UINT8);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has one byte of data ID:%X (%lld)", PID.id, Data);
			break;
		case 0x04:
			Prop.Data = *(UINT16*)Local_Ptr;
			Local_Ptr += sizeof(UINT16);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has two bytes of data ID:%X (%lld)", PID.id, Data);
			break;
		case 0x05:
			Prop.Data = *(UINT32*)Local_Ptr;
			Local_Ptr += sizeof(UINT32);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has four bytes of data ID:%X (%lld)", PID.id, Data);
			break;
		case 0x06:
			Prop.Data = *(UINT64*)Local_Ptr;
			Local_Ptr += sizeof(UINT64);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has eight bytes of data ID:%X (%lld)", PID.id, Data);
			break;
		case 0x07:
			Len = *(UINT32*)Local_Ptr;
			Prop.pData = malloc(Len);
			Local_Ptr += sizeof(UINT32);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has %d (+4) bytes of data ID:%X (%s)", Len, PID.id, (char *)Local_Ptr);
			memcpy_s(Prop.pData, Len, Local_Ptr, Len);
			Local_Ptr += Len;
			break;
		case 0x08:
			Prop.Data = ObjectIDs[0];
			sprintf_s(LogBuffer, LB_SIZE, "...Property one object ID  ID:%X", PID.id);
			break;
		case 0x09:
			// TO DO
			Len = *(UINT32*)Local_Ptr;
			Local_Ptr += sizeof(UINT32);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has %d ID's (of %lld) ID:%X", Len, ObjectIDs.size(), PID.id);
			break;
		case 0x0A:
			// TO DO
			sprintf_s(LogBuffer, LB_SIZE, "...Property has one OSID ID:%X", PID.id);
			break;
		case 0x0B:
			// TO DO
			Len = *(UINT32*)Local_Ptr;
			Local_Ptr += sizeof(UINT32);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has %d OSID's (of %lld) ID:%X", Len, ObjectSpaceIDs.size(), PID.id);
			break;
		case 0x0C:
			// TO DO
			sprintf_s(LogBuffer, LB_SIZE, "...Property has one Context ID ID:%X", PID.id);
			break;
		case 0x0D:
			// TO DO
			Len = *(UINT32*)Local_Ptr;
			Local_Ptr += sizeof(UINT32);
			sprintf_s(LogBuffer, LB_SIZE, "...Property has %d Context ID's (of %lld) ID:%X", Len, ContextIDs.size(), PID.id);
			break;
		case 0x10:
			// TO DO
			sprintf_s(LogBuffer, LB_SIZE, "...Property has Array of property values:%X", PID.id);
			break;
		case 0x11:
			// TO DO
			sprintf_s(LogBuffer, LB_SIZE, "...Property has Child Property set ID:%X", PID.id);
			break;
		}
		DoLog(typeid(*this).name(), LogBuffer, LOG_DEBUG);
		Properties[Prop.id] = Prop;


		/*
		NOW, the Page series node DocObject (jcid 0x60008) has a property that is 0x1D63: Child graph space element nodes. This references the 
		ID in the Object Space ID's section which is a compact ID. We then use the global ID table in the object group list to look up the GUID
		
		SO... this GUID must refer to something... but what?
		I think this is another Object space manifest list reference from the root file node list reference
		*/
		if (jcid == jcidPageSeriesNode && PID.id == 0x1D63)
			PageObjectSpace = ObjectSpaceIDs[0];

	}

	return Local_Ptr;
}
