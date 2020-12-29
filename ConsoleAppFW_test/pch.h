//
// pch.h
// Header for standard system include files.
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include "gtest/gtest.h"

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
