
set("SOURCE_FILES" 
 "main.cpp"
 "PlatformDebug.cpp" 
 "PlatformDebug.h"
)

source_group("Test" ${SOURCE_FILES})

add_executable(GraphicsPlayground ${SOURCE_FILES})

add_subdirectory(Graphics)
add_subdirectory(Libraries)

target_include_directories(GraphicsPlayground PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
