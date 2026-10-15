#pragma once

#include <cassert>
#include <windows.h>

// C++/WinRT 使用 Clang 的 UUID 属性和标准断言。
#undef __uuidof
#undef _ASSERTE
#define _ASSERTE(expression) assert(expression)
