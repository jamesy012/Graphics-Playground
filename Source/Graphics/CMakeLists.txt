message("Adding Graphics")

set("GraphicsPlayground_Graphics_Vulkan" ON CACHE BOOL "Enables Vulkan")

set(GRAPHICS_SOURCE 
"${CMAKE_CURRENT_SOURCE_DIR}/Graphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Graphics.cpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/Helpers.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Swapchain.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Swapchain.cpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/Framebuffer.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Framebuffer.cpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/Window.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Window.cpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/Devices.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Devices.cpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/Image.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Image.cpp" 
"${CMAKE_CURRENT_SOURCE_DIR}/RenderPass.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/RenderPass.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Buffer.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Buffer.cpp" )

if(${GraphicsPlayground_Graphics_Vulkan})
    message("Adding Vulkan")
    find_package(Vulkan REQUIRED)
    message(STATUS ${Vulkan_FOUND})
    message(STATUS ${Vulkan_INCLUDE_DIRS})
    message(STATUS ${Vulkan_LIBRARIES})
    message(STATUS ${Vulkan_VERSION})
    message(STATUS ${Vulkan_INCLUDE_DIR})
    message(STATUS ${Vulkan_LIBRARY})
    message(STATUS ${Vulkan_GLSLC_EXECUTABLE})
    message(STATUS ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
    target_link_libraries(GraphicsPlayground ${Vulkan_LIBRARYS})
    include_directories(AFTER ${Vulkan_INCLUDE_DIRS})

    file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/Shaders/*.frag"  
    "${PROJECT_SOURCE_DIR}/Shaders/*.vert"
    "${PROJECT_SOURCE_DIR}/Shaders/*.comp"
    )

    message("setting up shader compiling")
    foreach(SHADER ${GLSL_SOURCE_FILES})
      get_filename_component(FILE_NAME ${SHADER} NAME)
       set(SPIRV "${PROJECT_SOURCE_DIR}/WorkDir/Shaders/${FILE_NAME}.spv")
       message(STATUS "compiling ${SHADER} to ${SPIRV}")
       add_custom_command(
          OUTPUT ${SPIRV}
          COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/WorkDir/Shaders/"
          COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V "${SHADER}" -o "${SPIRV}"
          DEPENDS ${SHADER})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach()

    add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${GLSL_SOURCE_FILES}
    )

    add_dependencies(GraphicsPlayground Shaders)
endif()

target_sources(GraphicsPlayground PRIVATE ${GRAPHICS_SOURCE})
