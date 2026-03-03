// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING 

#pragma warning ( push )
#pragma warning( disable : 26495 )


#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <type_traits>
#include <typeinfo>
#include <vector>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <string>
#include <format>
#include <wtypes.h>
#include <map>
#include <thread>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

#pragma warning( pop )
