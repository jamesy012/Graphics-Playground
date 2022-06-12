#pragma once

#include <assert.h>
#include <string>

//c++ 20
#include <format>
#include <source_location>

#define ASSERT(x) assert(x);

namespace LOGGER {
	//does the actual log to terminal & Visual studio console
	void Log(const char* aMessage, const std::source_location location = std::source_location::current());
	void Log_Internal(const char* aMessage);

	//old log, still needed?
	//void Log(const char* aMessage, ...);

	//uses c++20 std::format to output to logs
	//("Position {},{},{}" ,x,y,z)  
	//("Path {0},{1},{0}{1}" ,"c:/", "Folder")  
	template<typename... Args> void Formated(const std::string aMessage, Args&&... args) {
		std::string output = std::vformat(aMessage, std::make_format_args(args...));
		Log_Internal(output.c_str());
	};

	template<typename... Args> void Formated(const char* aMessage, Args&&... args) {
		Formated(std::string(aMessage), args...);
	};
} // namespace LOGGER
