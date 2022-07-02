set("MAIN_FILES" "main.cpp" "PlatformDebug.cpp" "PlatformDebug.h")

add_executable(GraphicsPlayground ${MAIN_FILES})

set("GraphicsPlayground_Graphics_Vulkan"
    ON
    CACHE BOOL "Enables Vulkan")
set("GraphicsPlayground_Graphics_DX12"
    OFF
    CACHE BOOL "Enables DX12")
set("GraphicsPlayground_Enable_VR"
    OFF
    CACHE BOOL "Enables Open XR")
set("GraphicsPlayground_Enable_ImGui"
    ON
    CACHE BOOL "Enables ImGui")

add_subdirectory(Engine)
add_subdirectory(Graphics)
add_subdirectory(Libraries)

target_include_directories(GraphicsPlayground
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#message(
#  "Copying assets from ${CMAKE_SOURCE_DIR}/Assets/ to ${WORK_DIR_FOLDER}/${WORK_DIR_FOLDER_REL}/Assets"
#)
#add_custom_command(
#  TARGET GraphicsPlayground
#  PRE_BUILD
#  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Assets/
#          ${WORK_DIR_FOLDER}/${WORK_DIR_FOLDER_REL}/Assets)
#

file(GLOB_RECURSE _file_list RELATIVE "${CMAKE_SOURCE_DIR}/Assets/" "${CMAKE_SOURCE_DIR}/Assets/*")
 
foreach( each_file ${_file_list} )
  set(destinationfile "${WORK_DIR_FOLDER}/${WORK_DIR_FOLDER_REL}/Assets/${each_file}")
  set(sourcefile "${CMAKE_SOURCE_DIR}/Assets/${each_file}")
  if(NOT EXISTS ${destinationfile} OR ${sourcefile} IS_NEWER_THAN ${destinationfile})
    message("Copying ${sourcefile} to ${destinationfile}")
    get_filename_component(destinationdir ${destinationfile} DIRECTORY)
    file(COPY ${sourcefile} DESTINATION ${destinationdir})
  endif()
endforeach(each_file)
