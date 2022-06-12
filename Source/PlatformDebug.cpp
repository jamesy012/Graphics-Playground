#include "PlatformDebug.h"

#include <stdarg.h>

#if PLATFORM_WINDOWS
#	include <Windows.h>
#endif

//#include <cstring>
//#include <cstdio>
#include <format>

#if PLATFORM_WINDOWS
#	define LOGOUTPUT(x) OutputDebugStringA(x);
#else
#	define LOGOUTPUT(x)
#endif

namespace LOGGER {

	void Log_Internal(const char* aMessage) {
		printf(aMessage);
		LOGOUTPUT(aMessage);
	}
	void Log(const char* aMessage, const std::source_location location) {
		//File: F:\proj\Engines\Graphics-Playground\Source\Graphics\Pipeline.cpp(43:11) Create:
		//Formated("File: {}({}:{}) {}:\n\t", location.file_name(), location.line(), location.column(), location.function_name());
		Log_Internal(aMessage);
	}
} // namespace LOGGER