
set("SOURCE_FILES" 
 "main.cpp"
 "PlatformDebug.cpp" 
 "PlatformDebug.h"
 "Engine/FileIO.cpp" 
 "Engine/FileIO.h"
)

source_group("Test" ${SOURCE_FILES})

add_executable(GraphicsPlayground ${SOURCE_FILES})

set(GraphicsPlayground_Enable_VR OFF CACHE BOOL "Enables Open XR")

add_subdirectory(Graphics)
add_subdirectory(Libraries)

target_include_directories(GraphicsPlayground PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
