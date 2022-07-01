message("Adding Graphics")

# ~~~
# GRAPHICS CODE

if(GraphicsPlayground_Enable_ImGui)
  set(IMGUI_GRAPHICS_SOURCES "ImGuiGraphics.h"
                             "ImGuiGraphics.cpp")
endif()

if(GraphicsPlayground_Enable_VR)
  set(VR_GRAPHICS_SOURCES "VRGraphics.h"
                          "VRGraphics.cpp")
endif()

set(GRAPHICS_SOURCE
    "Graphics.h"
    "Graphics.cpp"
    "Helpers.h"
    "Swapchain.h"
    "Swapchain.cpp"
    "Framebuffer.h"
    "Framebuffer.cpp"
    "Devices.h"
    "Devices.cpp"
    "Image.h"
    "Image.cpp"
    "RenderPass.h"
    "RenderPass.cpp"
    "Buffer.h"
    "Buffer.cpp"
    "Pipeline.h"
    "Pipeline.cpp"
    "RenderTarget.h"
    "RenderTarget.cpp"
    "Material.h"
    "Material.cpp"
    "Screenspace.h"
    "Screenspace.cpp"
    "Mesh.h"
    "Mesh.cpp"
    "Model.h"
    "Model.cpp"
    "Conversions.h"
    "Loaders/LoaderBase.h"
    "Loaders/Loader.cpp"
    "Loaders/AssimpLoader.h"
    "Loaders/TinygltfLoader.h"
    ${IMGUI_GRAPHICS_SOURCES}
    ${VR_GRAPHICS_SOURCES})

target_sources(GraphicsPlayground PRIVATE ${GRAPHICS_SOURCE})

# ~~~
# VULKAN

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
  target_link_libraries(GraphicsPlayground ${Vulkan_LIBRARIES})
  include_directories(AFTER ${Vulkan_INCLUDE_DIRS})
endif()
