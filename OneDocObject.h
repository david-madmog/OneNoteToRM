#pragma once
#include "OneDocFileChunk.h"

typedef struct one_ObjectStreamHeader {
	unsigned int Count : 24; //An unsigned integer that specifies the number of CompactID structures in the stream that contains this ObjectSpaceObjectStreamHeader structure.
	unsigned int Reservred : 6; //MUST be zero, and MUST be ignored.
	unsigned int ExtendedStreamsPresent : 1; //specifies whether the ObjectSpaceObjectPropSet structure contains  any additional streams of data following this stream of data
	unsigned int OsidStreamNotPresent : 1; //specifies whether the ObjectSpaceObjectPropSet structure does not contain OSIDs or ContextIDs fields.
} ONE_ObjectStreamHeader;

typedef struct one_PropertyID {
	unsigned int id : 26; //specifies the identity of this property. The meanings of the id field values are specified in [MS-ONE] section 2.1.12
	unsigned int type : 5; // property type and the size and location of the data for this property
	unsigned int boolValue : 1;
} ONE_PropertyID;

typedef struct one_compactID {
	unsigned int n : 8; // specifies the value of the ExtendedGUID.n field
	unsigned int Index : 24; //specifies the index in the global identification table. The GUID that corresponds to this index provides the value for the ExtendedGUID.guid field.
} ONE_compactID;

typedef struct one_Property {
	int id;
	UINT64 Data;
	void* pData;
} ONE_Property;

class OneDocObject : public OneDocFileChunk
{
private:
	UINT64 Data=0;
	std::vector<UINT32> ObjectIDs;
	std::vector<ONE_compactID> ObjectSpaceIDs;
	std::vector<UINT32> ContextIDs;
	JCID jcid;
public:
	ONE_compactID PageObjectSpace = { 0, 0 };
	std::map<int, ONE_Property> Properties;

	OneDocObject(JCID pjcid) { Magic = 0; jcid = pjcid; };
	
	void * ParseObjectPropSet();

};

