message("Adding Libraries")

#~ OpenXR
set("GraphicsPlayground_Enable_VR" OFF CACHE BOOL "Enables Open XR")
if(${GraphicsPlayground_Enable_VR})
message("Adding OpenXR")
set("OpenXR-Link" openxr_loader)
target_compile_definitions(GraphicsPlayground PUBLIC ENABLE_XR)
if(${GraphicsPlayground_Graphics_Vulkan})
   target_compile_definitions(GraphicsPlayground PUBLIC XR_USE_GRAPHICS_API_VULKAN)
endif()
#find_package(OpenXR)
add_subdirectory(OpenXR-SDK)
target_include_directories(GraphicsPlayground PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/OpenXR-SDK/include")
endif()

#~~~ Other sub directories
#add_subdirectory(assimp EXCLUDE_FROM_ALL)
   message("Adding GLFW")
add_subdirectory(glfw EXCLUDE_FROM_ALL)
   message("Adding GLM")
add_subdirectory(glm EXCLUDE_FROM_ALL)
   message("Adding VMA")
add_subdirectory(VulkanMemoryAllocator EXCLUDE_FROM_ALL)

#add_subdirectory(Libraries/googleTest EXCLUDE_FROM_ALL)
#set(BUILD_SHARED_LIBS OFF)
#add_subdirectory(Libraries/glslang EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/SPIRV-Headers EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/SPIRV-Tools EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/shaderc EXCLUDE_FROM_ALL)

#~~~ IMGUI
set("GraphicsPlayground_Enable_ImGui" ON CACHE BOOL "Enables ImGui")
if(GraphicsPlayground_Enable_ImGui)
   target_compile_definitions(GraphicsPlayground PUBLIC ENABLE_IMGUI)
   message("Adding ImGui")

   set(IMGUI_PATH "imgui")

   set(IMGUI_SOURCES
      "${IMGUI_PATH}/imgui.cpp"
      "${IMGUI_PATH}/imgui_demo.cpp"
      "${IMGUI_PATH}/imgui_draw.cpp"
      "${IMGUI_PATH}/imgui_tables.cpp"
      "${IMGUI_PATH}/imgui_widgets.cpp"
   )
   
   set(IMGUI_HEADERS
      "${IMGUI_PATH}/imconfig.h"
      "${IMGUI_PATH}/imgui.h"
      "${IMGUI_PATH}/imgui_internal.h"
      "${IMGUI_PATH}/imstb_rectpack.h"
      "${IMGUI_PATH}/imstb_textedit.h"
      "${IMGUI_PATH}/imstb_truetype.h"
   )

   #if GLFW
   set(IMGUI_SOURCES ${IMGUI_SOURCES} "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp")
   set(IMGUI_HEADERS ${IMGUI_HEADERS} "${IMGUI_PATH}/backends/imgui_impl_glfw.h")

   #if Vulkan
   set(IMGUI_SOURCES ${IMGUI_SOURCES} "${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp")
   set(IMGUI_HEADERS ${IMGUI_HEADERS} "${IMGUI_PATH}/backends/imgui_impl_vulkan.h")

   target_sources(GraphicsPlayground PRIVATE ${IMGUI_SOURCES} ${IMGUI_HEADERS})
   target_include_directories(GraphicsPlayground PRIVATE ${IMGUI_PATH})
   #target_include_directories(GraphicsPlayground PRIVATE ${IMGUI_PATH}/imgui.h)
   target_include_directories(GraphicsPlayground PRIVATE ${IMGUI_PATH}/backends)
endif()

message("Adding stb")
set(STB_SOURCES
"stb/stb_image.h"
)
target_sources(GraphicsPlayground PRIVATE ${STB_SOURCES})
target_include_directories(GraphicsPlayground PRIVATE "stb")

#~~~ Target links
message("Linking Targets")

target_include_directories(GraphicsPlayground PRIVATE "VulkanMemoryAllocator/include")

target_link_libraries(GraphicsPlayground
#assimp 
glfw 
glm
VulkanMemoryAllocator
${OpenXR-Link}
) 