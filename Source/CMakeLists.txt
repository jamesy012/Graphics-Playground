add_subdirectory(Libraries)

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

target_include_directories(GraphicsPlayground PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_WINDOWS=1)

target_include_directories(GraphicsPlayground PRIVATE ${EXTERNAL_INCLUDE_LIBRARIES})
target_link_libraries(GraphicsPlayground ${EXTERNAL_LINK_LIBRARIES})
