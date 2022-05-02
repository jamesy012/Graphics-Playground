
set("SOURCE_FILES" 
 "main.cpp"
 "PlatformDebug.cpp" 
 "PlatformDebug.h"
)

source_group("Test" ${SOURCE_FILES})

find_package(Vulkan REQUIRED)
add_subdirectory(Graphics)
message("Added Graphics Folder with ${GRAPHICS_SOURCE}")
message("Added IMGUI_HEADERS Folder with ${IMGUI_HEADERS}")
message("Added EXTERNAL_LINK_LIBRARIES Folder with ${EXTERNAL_LINK_LIBRARIES}")

set("SOURCE_COMBINED" ${SOURCE_FILES} ${GRAPHICS_SOURCE})
add_executable(GraphicsPlayground ${SOURCE_COMBINED})

add_subdirectory(Libraries)

target_include_directories(GraphicsPlayground PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(PLATFORM_WINDOWS 0)
set(PLATFORM_APPLE 0)
set(PLATFORM_LINUX 0)

if(WIN32)
set(PLATFORM_WINDOWS 1)
endif()
if(APPLE)
set(PLATFORM_APPLE 1)
endif()
if(UNIX AND NOT APPLE)  # for Linux, BSD, Solaris, Minix
set(PLATFORM_LINUX 1)
endif()

target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_NAME=${CMAKE_SYSTEM_NAME})
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_WINDOWS=${PLATFORM_WINDOWS})
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_APPLE=${PLATFORM_APPLE})
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_LINUX=${PLATFORM_LINUX})

message(${EXTERNAL_INCLUDE_LIBRARIES})
target_include_directories(GraphicsPlayground PRIVATE ${EXTERNAL_INCLUDE_LIBRARIES})
target_link_libraries(GraphicsPlayground ${EXTERNAL_LINK_LIBRARIES})
