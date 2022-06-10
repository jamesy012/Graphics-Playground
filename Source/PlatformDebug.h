#pragma once

#include <assert.h>

#define ASSERT(x) assert(x);

namespace LOG {

	void Log(const char* aMessage, ...);
	//void LogLine(const char* aMessage, ...);
	void LogLine(const char* aMessage);
} // namespace LOG
