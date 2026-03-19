/*******************************************************************************

	Header.h

	Standard include set for all .cpp modules

	(C) David Poirier 2026

********************************************************************************/

#pragma once

#define _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING 

#pragma warning ( push )
#pragma warning( disable : 26495 )

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// C RunTime Header Files

#include <algorithm>
#include <array>
#include <assert.h>
#include <codecvt>
#include <cstdio>
#include <format>
#include <fstream>
#include <iostream>
#include <locale>
#include <malloc.h>
#include <map>
#define _USE_MATH_DEFINES
#include <math.h>
#include <memory.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <wtypes.h>

//#include <afxwin.h>   // must come first
#include <Unknwn.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")

#pragma warning( pop )
