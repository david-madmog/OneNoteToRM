#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cpprest/filestream.h>

/*******************************************************************************

	RMAPI.h

	Header for wrapping RM API calls for getting, putting and listing RM docs
	Basically just shells out to RMAPI executable do do the work

	see https://github.com/ddvk/rmapi for download and documentation on RMAPI

	(C) David Poirier 2026

********************************************************************************/

#ifndef IB_SIZE
#define IB_SIZE 1024
#endif // !IB_SIZE

constexpr auto Sep = L"\\";


class RMAPI
{
private:
	static std::string exec(const char* cmd); 
	static int filecopy(std::string from, std::string to);

	std::string TokenIniFileName;
	std::wstring wIniFileName;
	std::wstring CatalogCache;
	long long LastGeneration = 0;

	char* DeviceToken = NULL;
	std::wstring* UserToken = NULL;

	wchar_t StorageRoot[IB_SIZE];
	wchar_t StorageDataPath[IB_SIZE];

	//	char* LoginCode = NULL;
	//std::string LoginCode;
	std::wstring ServicePath = L"";

	int RegisterDevice(const char* deviceCode);
	int GetUserToken(char* DeviceToken);
	//void GetServicePath();

	enum NodeType {
		Unknown,
		Directory,
		FileData,
		Content,
		Metadata
	};

	typedef struct sDocNode {
		std::wstring ID = L"";
		std::wstring UUID=L"";
		NodeType Type = NodeType::Unknown;
		std::wstring Parent=L"";
		std::wstring UnitName=L"";
		std::wstring Path=L"";
		std::wstring Gen = L"";

		friend std::wostream& operator<<(std::wostream& os, const sDocNode RHS)
		{
			if (RHS.Path.empty())
				os << RHS.UnitName << L"(" << RHS.UUID << L")";
			else
				os << RHS.Path << L"|" << RHS.ID << L"," << RHS.UUID;
//				os << RHS.Path;
			return os;
		}

	public: 
		void SaveToCache(const wchar_t* CatalogCache) const;
		bool LoadFromCache(std::wstring ID, const wchar_t* CatalogCache);
	} DocNode;

	std::wstring* GetStorage(const wchar_t* path, const wchar_t* node, const wchar_t* RMFilename);

	std::wstring RecursePath(std::unordered_map<std::wstring, DocNode>& Nodes, DocNode Node);
	void LoadRootNodes(std::unordered_map<std::wstring, DocNode>& Nodes, std::wstring& NodeID);
	void LoadFileData(DocNode* Node);
	void LoadMetadata(std::wstring& NodeID, std::wstring& NodeUUID, DocNode* Node);
//		void WalkTree(std::unordered_map<std::string, DocNode>& Nodes, std::wstring& NodeID, std::wstring& NodeUUID, NodeType type, DocNode* Node);

public:
	RMAPI();
	~RMAPI();

	bool EnsureConnected(void);
	void SetDeviceCode(const wchar_t* DeviceCodeW);

	static void GetDoc(std::string Name); // Old (RMAPI) version
	//	static void ListDocsStringToVector(std::string ListDocsString, std::vector<std::wstring>& Docs);
	std::wstring ListDocsString();
	std::wstring* GetDataStorage(const wchar_t* node, const wchar_t* RMFilename);
	concurrency::streams::istream GetPage(const wchar_t* node, const wchar_t* RMFilename);


	static void SaveDoc(std::string Name, std::string path);
	static void CopyDoc(std::string Name);
};

