#pragma once

#include "RMDocFile.h"
#include "WindowRMPage.h"
#include "RMBlock.h"
#include "RMBlockTypes.h"

template<class PageType> class RMTestFileBuilder : public RMDocFile<PageType>
{
private:
	void LoadMetaData();

public:
	int Build();
};

