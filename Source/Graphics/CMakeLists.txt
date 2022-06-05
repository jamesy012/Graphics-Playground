message("Adding Graphics")

#~~~~~ GRAPHICS CODE

if(GraphicsPlayground_Enable_ImGui)
set(IMGUI_GRAPHICS_SOURCES 
"${CMAKE_CURRENT_SOURCE_DIR}/ImGuiGraphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/ImGuiGraphics.cpp" 
)
endif()

if(GraphicsPlayground_Enable_VR)
set(VR_GRAPHICS_SOURCES 
"${CMAKE_CURRENT_SOURCE_DIR}/VRGraphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/VRGraphics.cpp" 
)
endif()

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
"${CMAKE_CURRENT_SOURCE_DIR}/Buffer.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Pipeline.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/RenderTarget.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/RenderTarget.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/Material.h" 
"${CMAKE_CURRENT_SOURCE_DIR}/Material.cpp"
${IMGUI_GRAPHICS_SOURCES}
${VR_GRAPHICS_SOURCES}
 )

target_sources(GraphicsPlayground PRIVATE ${GRAPHICS_SOURCE})

#~~~~~~ VULKAN/SHADERS

set("GraphicsPlayground_Graphics_Vulkan" ON CACHE BOOL "Enables Vulkan")

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
endif()
