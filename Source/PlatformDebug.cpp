#include "PlatformDebug.h"

#include <stdarg.h>

#if PLATFORM_WINDOWS
#	include <Windows.h>
#endif

#include <cstring>
#include <cstdio>

#if PLATFORM_WINDOWS
#	define LOGOUTPUT(x) OutputDebugStringA(x);
#else
#	define LOGOUTPUT(x)
#endif

#define LOGCONSOLE(...)           \
	memset(buffer, 0, size);      \
	sprintf(buffer, __VA_ARGS__); \
	printf(buffer);               \
	LOGOUTPUT(buffer)
#define LOGCONSOLEVA(x, p_va_list) \
	memset(buffer, 0, size);       \
	vsprintf(buffer, p_va_list);   \
	printf(buffer);                \
	LOGOUTPUT(buffer)

namespace LOG {

	void Log(const char* aMessage, ...) {

		const unsigned int size	 = 1024 * 16;
		static char buffer[size] = {0};
		memset(buffer, 0, size);

		// Category
		//if (!mLogCat.empty())
		//{
		//	LOGCONSOLE("%s%s", mLogCat.c_str(), ":\t")
		//}

		memset(buffer, 0, size);

		// indent
		//std::string indent;
		//for (int i = 0; i < mPrintIndent; ++i)
		//{
		//	indent += '\t';
		//}
		//LOGCONSOLE(indent.c_str());

		// message/arguments
		va_list argptr;
		va_start(argptr, aMessage);
		vprintf(aMessage, argptr);

		vsprintf(buffer, aMessage, argptr);
		LOGOUTPUT(buffer);

		va_end(argptr);
	}

	//void LogLine(const char* aMessage, ...) {
	void LogLine(const char* aMessage) {

		int size	  = strlen(aMessage) + 1; //include \0
		char* message = new char[size + 1]; //adding \n
		memcpy(message, aMessage, size);
		message[size - 1] = '\n';
		message[size]	  = '\0';

		//va_list args;
		//va_start(args, aMessage);
		//Log(message, args);
		//va_end(args);
		Log(message);

		delete[] message;
	}
} // namespace LOG