#pragma once
#include <iostream>
#include <source_location>

#ifdef _DEBUG
#define DEBUG_PRINT(ToPrint) \
		std::cout << "--------" << std::endl; \
		std::source_location CurrentLocation = std::source_location::current(); \
		std::cout << "File: " << CurrentLocation.file_name() << std::endl; \
		std::cout << "Function: " << CurrentLocation.function_name() << std::endl;\
		std::cout << "Line: " << CurrentLocation.line() << std::endl; \
		std::cout << "What: " << ToPrint << std::endl; \
		std::cout << "--------" << std::endl;

#define DEBUG_FUNC(Callable) Callable()
#else
#define DEBUG_PRINT(ToPrint) do { } while(0)
#define DEBUG_FUNC(Callable) do { } while(0)
#endif