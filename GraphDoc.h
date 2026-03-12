#pragma once
#include "framework.h"
#include "secrets.h"
#include "GraphAPI.h"
#include "OneNoteToRM.h"

#pragma warning ( push )
#pragma warning( disable : 4005 26819)
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
#pragma warning ( pop )

using njson = nlohmann::json;

template<class PageType> class GraphDoc : public Drawable
{
private:
	void DeletePages(wchar_t* SectionID);
	wchar_t* FindDocID(const std::string& NotebookName, const std::string& SectionName);

	GraphAPI* API;

protected:
	std::vector<PageType* > Pages;

public:
	GraphDoc(GraphAPI* pAPI);
	~GraphDoc();

	int LoadDoc(const std::string& NotebookName, const std::string& SectionName);
	int SaveDoc(const std::string& NotebookName, const std::string& SectionName);
	int LoadPages(wchar_t* SectionID);
	int SaveDoc(wchar_t* SectionID);

	void DrawPage(void* DrawDetails, int page);

	void AddPage(PageType* Page) { Pages.push_back(Page); }
};

