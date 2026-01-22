#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>

#include <string>
#include <iostream>
#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <fstream>

#include "PatternScanner.hpp"
#include "MinHookManager.h"
#include "MinHook/include/MinHook.h"

#pragma comment(lib, "MinHook/lib/libMinHook.x64.lib")