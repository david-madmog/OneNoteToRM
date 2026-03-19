#pragma once
#include "ONEPage.h"

/*******************************************************************************

    WindowONEPage.h

    Header for display of a ONEPage

    This is a subclass of ONEPage. It uses the ONEPage subclass to load the page,
        and then renders it to the DC passed in as part of the DrawDetails

    (C) David Poirier 2026

*******************************************************************************/


class WindowONEPage : public ONEPage
{
public:
	void DrawPage(void* DrawDetails);
};

