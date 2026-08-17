#pragma once
#include <string>
#include <vector>
#include "GraphAPI.h"
#include "OneNoteToRM.h"

/*******************************************************************************

	GraphDoc.h

	Header for base class implementing OneNote graph document.
	Includes functions specific to loading and saving OneNote pages and is intended to 
		be subclassed with something that can render the page - e.g. to draw to a Windows
		DC or to render into some other format

	(C) David Poirier 2026

********************************************************************************/


template<class PageType> class GraphDoc : public Drawable
{
private:
	void DeletePages(const wchar_t* SectionID);
	wchar_t* FindDocID(const std::string& NotebookName, const std::string& SectionName);
	wchar_t* CreateSection(const std::string& NotebookName, const std::string& SectionName);

	GraphAPI* API;

protected:
	std::vector<PageType* > Pages;

public:
	GraphDoc(GraphAPI* pAPI);
	~GraphDoc();

	int LoadDoc(const std::string& NotebookName, const std::string& SectionName);
	int SaveDoc(const std::string& NotebookName, const std::string& SectionName);
	int LoadDoc(const wchar_t* SectionID);
	int SaveDoc(const wchar_t* SectionID);

	void DrawPage(void* DrawDetails, int page);

	void AddPage(PageType* Page) { Pages.push_back(Page); }
	time_t LastEditTime();

	std::string Name = "";
};

