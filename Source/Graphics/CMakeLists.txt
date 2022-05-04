set("Graphics_Vulkan" ON CACHE BOOL "Enables Vulkan")

set(GRAPHICS_SOURCE 
"${CMAKE_CURRENT_SOURCE_DIR}/Graphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/Graphics.cpp" 
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
CACHE INTERNAL "")



set(EXTERNAL_LINK_LIBRARIES ${EXTERNAL_LINK_LIBRARIES} Vulkan::Vulkan CACHE INTERNAL "")
include_directories(AFTER ${VULKAN_PATH}/Include)

#source_group("Graphics2" ${GRAPHICS_SOURCE} )
message("Added Graphics Folder with ${GRAPHICS_SOURCE}")