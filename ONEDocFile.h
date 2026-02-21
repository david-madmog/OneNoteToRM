#pragma once
#include "ONEPage.h"
#include "OneDocObject.h"
#include "framework.h"

#pragma pack(1)


template<class PageType> class ONEDocFile
{
private:
    std::vector<PageType*>Pages;
//	std::vector<void*>Buffers;

public:
    ONEDocFile() { static_assert(std::is_base_of<ONEPage, PageType>::value, "Doc file must be based on PageType derived from ONEPage"); }
//	~ONEDocFile();
    int ExtractFromONE(const char* FileName);
//    int SaveRMsToZip(const char* FileName);
    void DrawPage(void* DrawDetails, int pageID);

};

