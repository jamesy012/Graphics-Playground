#pragma once

#include <assert.h>
#include <string>

//c++ 20
#if __cpp_lib_format
#	include <format>
#else
#	include <iostream>
#endif
#if __cpp_lib_source_location
#	include <source_location>
#endif

#define ASSERT(x) assert(x);

namespace LOGGER {
	//does the actual log to terminal & Visual studio console
#if __cpp_lib_source_location
	void Log(const char* aMessage, const std::source_location location = std::source_location::current());
#else
	void Log(const char* aMessage);
#endif
	void Log_Internal(const char* aMessage);

#if __cpp_lib_format == false
	//could problably actually make my own replacement
	//doesnt seem overally hard to hack about, given how this function works
	template<typename... Args> void QuickNonFormatLog(Args&&... args) {
		int counter = 0;
		//change this to LOGOUTPUT?
		((std::cout << "\t{" << counter++ << "} " << std::forward<Args>(args) << "\n"), ...);
	}
	//template<typename Arg, typename... Args> void QuickNonFormatLog(Arg&& arg, Args&&... args) {
	//	std::cout << "\t" << std::forward<Arg>(arg);
	//	((std::cout << ", " << std::forward<Args>(args)), ...);
	//	std::cout << std::endl;
	//}
#endif //__cpp_lib_format == false

	//old log, still needed?
	//void Log(const char* aMessage, ...);

	//uses c++20 std::format to output to logs
	//("Position {},{},{}" ,x,y,z)
	//("Path {0},{1},{0}{1}" ,"c:/", "Folder")
	template<typename... Args> void Formated(const std::string aMessage, Args&&... args) {
#if __cpp_lib_format
		std::string output = std::vformat(aMessage, std::make_format_args(args...));
		Log_Internal(output.c_str());
#else //__cpp_lib_format
		const std::string output = "(*) " + aMessage;
		Log_Internal(output.c_str());
		QuickNonFormatLog(args...);
		//QuickNonFormatLog(std::forward<Args>(args));
#endif //__cpp_lib_format
	};

	template<typename... Args> void Formated(const char* aMessage, Args&&... args) {
		Formated(std::string(aMessage), args...);
	};
} // namespace LOGGER
