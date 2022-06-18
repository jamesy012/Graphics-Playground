set(GLSL_SOURCE_FILES
    "/ImGui.vert"
    "/ImGui.frag"
    "/Screenspace/Screenspace.vert"
    "/Screenspace/Image.frag"
    "/Screenspace/ImageArray.frag"
    "/MeshTest.vert"
    "/MeshTest.frag")
set(GLSL_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Include/test.inc")

# file(GLOB_RECURSE GLSL_SOURCE_FILES "${PROJECT_SOURCE_DIR}/Shaders/*.frag"
# "${PROJECT_SOURCE_DIR}/Shaders/*.vert" "${PROJECT_SOURCE_DIR}/Shaders/*.comp"
# )

message("setting up shader compiling")
message(STATUS "GLSL exe ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")

foreach(SHADER ${GLSL_SOURCE_FILES})
  message(STATUS "SHADER -  ${SHADER}")
  get_filename_component(FILE_NAME ${SHADER} NAME)
  get_filename_component(rel2 ${SHADER} ABSOLUTE)
  file(TO_CMAKE_PATH ${SHADER} rel)
  # cmake_path(GET ${SHADER} FILENAME  rel3)
  cmake_path(GET SHADER FILENAME SHADER_NAME)
  cmake_path(GET SHADER PARENT_PATH SHADER_FOLDER)

  set(SPIRV_OUTPUT ${WORK_DIR_FOLDER}/WorkDir/Shaders${SHADER_FOLDER})

  message(STATUS "Input ${CMAKE_CURRENT_SOURCE_DIR}${SHADER}")
  message(STATUS "SHADER_NAME ${SHADER_NAME}")
  message(STATUS "SHADER_FOLDER ${SHADER_FOLDER}")
  message(STATUS "SPIRV_OUTPUT ${SPIRV_OUTPUT}")

  # set(SPIRV "${WORK_DIR_FOLDER}/WorkDir/Shaders/${FILE_NAME}.spv")
  set(SPIRV "${WORK_DIR_FOLDER}/WorkDir/Shaders${SHADER}.spv")
  message(STATUS "compiling ${CMAKE_CURRENT_SOURCE_DIR}${SHADER} to ${SPIRV}")

  file(MAKE_DIRECTORY "${WORK_DIR_FOLDER}/WorkDir/Shaders${SHADER_FOLDER}")

  if(${GraphicsPlayground_Enable_VR})
    set(VR_ENABLED --define-macro "VR_ENABLED=1")
  endif()

  # can add -S <Stage> by getting file extension

  add_custom_command(
    OUTPUT ${SPIRV}
    COMMENT "Building Shader: ${SPIRV}"
    # COMMAND ${CMAKE_COMMAND} -E make_directory "${WORK_DIR_FOLDER}/WorkDir/"
    # COMMAND ${CMAKE_COMMAND} -E make_directory
    # "${WORK_DIR_FOLDER}/WorkDir/Shaders/" COMMAND
    # ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V "${SHADER}" -o "${SPIRV}"
    COMMAND
      ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V
      "${CMAKE_CURRENT_SOURCE_DIR}${SHADER}" -o "${SPIRV}" -g
      -I"${CMAKE_CURRENT_SOURCE_DIR}/include/" --define-macro "TEST=10"
      ${VR_ENABLED}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}${SHADER}
    DEPENDS "${GLSL_INCLUDE_FILES}" # update all shaders when a include change
                                    # happens, is there a better way for this?
  )

  message(
    STATUS
      "${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${CMAKE_CURRENT_SOURCE_DIR}${SHADER} -o ${SPIRV} -g"
  )
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  list(APPEND GLSL_ABSO_BINARY_FILES ${CMAKE_CURRENT_SOURCE_DIR}${SHADER})
endforeach()

add_custom_target(
  Shaders
  DEPENDS ${SPIRV_BINARY_FILES}
  SOURCES ${GLSL_ABSO_BINARY_FILES})

add_dependencies(GraphicsPlayground Shaders)
