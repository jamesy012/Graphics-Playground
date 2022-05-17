cmake_minimum_required(VERSION 3.21.0)
message("----  Starting ----")
project(GraphicsPlayground VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#~~~ Get Platform
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

message(STATUS ${CMAKE_GENERATOR})

if( NOT APPLE) 
set(WORK_DIR_FOLDER_REL "../")
endif()
get_filename_component(WORK_DIR_FOLDER "${CMAKE_BINARY_DIR}/${WORK_DIR_FOLDER_REL}" ABSOLUTE)
message("BUILD_OUTPUT ${WORK_DIR_FOLDER}")

#~~~ make project
add_subdirectory(Source)

#~~~ Apply Platform to project
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_NAME=${CMAKE_SYSTEM_NAME})
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_WINDOWS=${PLATFORM_WINDOWS})
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_APPLE=${PLATFORM_APPLE})
target_compile_definitions(GraphicsPlayground PUBLIC PLATFORM_LINUX=${PLATFORM_LINUX})

#need a better way to determin this.. or make them work the same?
if (APPLE)
message(STATUS "using visual studio APPLE as generator")
message("setting WORK_DIR_REL to \"${WORK_DIR_FOLDER_REL}/\"")
target_compile_definitions(GraphicsPlayground PUBLIC WORK_DIR_REL=\"${WORK_DIR_FOLDER_REL}Build/\")
elseif (CMAKE_GENERATOR MATCHES "Ninja")
message(STATUS "using visual studio as generator")
message("setting WORK_DIR_REL to \"${WORK_DIR_FOLDER_REL}/../\"")
target_compile_definitions(GraphicsPlayground PUBLIC WORK_DIR_REL=\"${WORK_DIR_FOLDER_REL}/../\")
else()
message(STATUS "using visual studio code")
message("setting WORK_DIR_REL to \"${WORK_DIR_FOLDER_REL}\"")
target_compile_definitions(GraphicsPlayground PUBLIC WORK_DIR_REL=\"${WORK_DIR_FOLDER_REL}\")
endif()

#set(IMGUI_PATH "Libraries/imgui")
#set(STB_PATH "Libraries/stb")
#set(VMA_PATH "Libraries/VulkanMemoryAllocator/include")
#
#set(IMGUI_SOURCES
#    "${IMGUI_PATH}/imgui.cpp"
#    "${IMGUI_PATH}/imgui_demo.cpp"
#    "${IMGUI_PATH}/imgui_draw.cpp"
#    "${IMGUI_PATH}/imgui_tables.cpp"
#    "${IMGUI_PATH}/imgui_widgets.cpp"
#    "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp"
#    "${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp"
#)
#
#set(IMGUI_HEADERS
#    "${IMGUI_PATH}/imconfig.h"
#    "${IMGUI_PATH}/imgui.h"
#    "${IMGUI_PATH}/imgui_internal.h"
#    "${IMGUI_PATH}/imstb_rectpack.h"
#    "${IMGUI_PATH}/imstb_textedit.h"
#    "${IMGUI_PATH}/imstb_truetype.h"
#    "${IMGUI_PATH}/backends/imgui_impl_glfw.h"
#    "${IMGUI_PATH}/backends/imgui_impl_vulkan.h"
#)
#
#file(GLOB_RECURSE CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp)
#file(GLOB_RECURSE HPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h)
#
#list(APPEND SHADER_FILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/workdir/Shaders/*.vert )
#list(APPEND SHADER_FILE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/workdir/Shaders/*.frag )
#
#file(GLOB_RECURSE SHADER_FILES ${SHADER_FILE_SOURCES})
#
#list(APPEND CPP_FILES ${IMGUI_SOURCES})
#list(APPEND HPP_FILES ${IMGUI_HEADERS})
#
#add_executable(VulkanPlayground ${CPP_FILES} ${HPP_FILES} ${SHADER_FILES})
#
#set_property(TARGET VulkanPlayground PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/workdir")
#set_property(TARGET VulkanPlayground PROPERTY VS_DEBUGGER_COMMAND "${CMAKE_SOURCE_DIR}/workdir")
#
#target_precompile_headers(VulkanPlayground PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source/stdafx.h)
#
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Source/)
#
#include_directories(${IMGUI_PATH})
#include_directories(${IMGUI_PATH}/backends)
#
#
##Vulkan
#
#find_package(Vulkan REQUIRED)
#
#target_link_libraries(VulkanPlayground PRIVATE Vulkan::Vulkan)
#include_directories(AFTER ${VULKAN_PATH}/Include)
#
##Other
#
#add_subdirectory(Libraries/assimp EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/glfw EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/glm EXCLUDE_FROM_ALL)
#
#add_subdirectory(Libraries/googleTest EXCLUDE_FROM_ALL)
##set(ENABLE_HLSL OFF)
##set(ENABLE_HLSL ON)
#set(BUILD_SHARED_LIBS OFF)
#add_subdirectory(Libraries/glslang EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/SPIRV-Headers EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/SPIRV-Tools EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/shaderc EXCLUDE_FROM_ALL)
#
##add_subdirectory(Libraries/VulkanMemoryAllocator EXCLUDE_FROM_ALL)
#
##find_library(DInput_Path dinput8)
#
#if(WIN32)
#   set(PLATFORM_FILES dxguid.lib dinput8.lib)
#endif()
#
#
#target_link_libraries(VulkanPlayground
#  PRIVATE assimp
#  PRIVATE glfw
#  PRIVATE glm
##  PRIVATE VulkanMemoryAllocator
#
#  PRIVATE glslang
##  PRIVATE SPIRV-Headers
##  PRIVATE SPIRV-Tools
#  PRIVATE shaderc
#
#  ${PLATFORM_FILES}
##  PRIVATE dxguid.lib
##  PRIVATE dinput8.lib
#)
#
#target_include_directories(VulkanPlayground PRIVATE ${STB_PATH} PRIVATE ${VMA_PATH})
#
##copy
##file(COPY ${<some_var>}
##    DESTINATION ${EXECUTABLE_OUTPUT_PATH})
#message("----  Copy? ----")
##windows glfw/assimp copy
#if(WIN32)
#add_custom_command(TARGET VulkanPlayground
#    POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/Libraries/glfw/src/glfw3d.dll "${CMAKE_CURRENT_BINARY_DIR}"
#    COMMENT "Copy dll file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
#)
#add_custom_command(TARGET VulkanPlayground
#    POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/Libraries/assimp/bin/assimp-vc143-mtd.dll "${CMAKE_CURRENT_BINARY_DIR}"
#    COMMENT "Copy dll file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
#)
#endif()
#
#
###imgui
##set(IMGUI_SOURCES
##    "${IMGUI_PATH}/*.cpp"
##)
##
##set(IMGUI_HEADERS
##    "${IMGUI_PATH}/*.h"
##)
##
##set(IMGUI_INC_PATH "${IMGUI_PATH}/")
##
##target_compile_definitions(VulkanPlayground
##    PRIVATE
##        IMGUI_IMPL_OPENGL_LOADER_GLAD=1
##)
##
##target_include_directories(VulkanPlayground
##    PUBLIC
##        "${IMGUI_INC_PATH}"
##        "${GLFW_INC_PATH}"
##        "${GLAD_INC_PATH}"
