#pragma once
#include <iostream>

#ifdef _DEBUG
#define DEBUG_FUNC(x) x()
#define DEBUG_PRINT(x) std::cout << x << "\n"; DebugBreak()
#else
#define DEBUG_FUNC(x) do { } while(0)
#define DEBUG_PRINT(x) do { } while(0)
#endif
