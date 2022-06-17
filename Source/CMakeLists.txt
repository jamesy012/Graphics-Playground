
set("MAIN_FILES" 
 "main.cpp"
 "PlatformDebug.cpp" 
 "PlatformDebug.h"
)

add_executable(GraphicsPlayground ${MAIN_FILES})

set("GraphicsPlayground_Enable_Vulkan" ON CACHE BOOL "Enables Vulkan")
set("GraphicsPlayground_Graphics_DX12" OFF CACHE BOOL "Enables DX12")
set("GraphicsPlayground_Enable_VR" OFF CACHE BOOL "Enables Open XR")
set("GraphicsPlayground_Enable_ImGui" ON CACHE BOOL "Enables ImGui")

add_subdirectory(Engine)
add_subdirectory(Graphics)
add_subdirectory(Libraries)

target_include_directories(GraphicsPlayground PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

message("Copying assets from ${CMAKE_SOURCE_DIR}/config/ to ${WORK_DIR_FOLDER}/WorkDir/Assets")
add_custom_command(TARGET GraphicsPlayground PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/Assets/ ${WORK_DIR_FOLDER}/WorkDir/Assets)