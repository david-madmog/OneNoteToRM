#pragma once

/*******************************************************************************

	ConversionConstants.H

	Sizing and scaling mappings between the RM and OneNote worlds. 
	Used by both ToRM and ToOne converters for consistency

	(C) David Poirier 2026

********************************************************************************/


#define RM_TO_ONE_XY_SCALE_FACTOR 20
#define RM_TO_ONE_THICK_FACTOR 10
#define RM_TO_ONE_LINE_FACTOR 20
#define RM_TO_ONE_PRESSURE_FACTOR 128

#define PAGE_SIZE_X 1404
#define PAGE_SIZE_Y 1872
#define RM_X_OFFSET (PAGE_SIZE_X / 2)


#define TEXT_X_START 50
#define TEXT_Y_START 150
#define LINES_Y_START 300

//#define X_SCALE 0.5
//#define Y_SCALE 0.5
#define X_SCALE 1
#define Y_SCALE 1
