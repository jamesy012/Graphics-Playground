message("Adding Libraries")

# means we dont need to link to dll's for these libraries
set(BUILD_SHARED_LIBS OFF)

# ~~~
# OpenXR
if(${GraphicsPlayground_Enable_VR})
  message("Adding OpenXR")
  if(PLATFORM_APPLE)
    add_definitions(-DXR_OS_APPLE)
  endif()
  set("OpenXR-Link" openxr_loader)
  target_compile_definitions(GraphicsPlayground PUBLIC ENABLE_XR)
  if(${GraphicsPlayground_Graphics_Vulkan})
    target_compile_definitions(GraphicsPlayground
                               PUBLIC XR_USE_GRAPHICS_API_VULKAN)
  endif()
  # find_package(OpenXR)
  add_subdirectory(OpenXR-SDK)
  target_include_directories(
    GraphicsPlayground PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/OpenXR-SDK/include")
endif()

# ~~~
# Other sub directories
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
message("Adding assimp")
add_subdirectory(assimp EXCLUDE_FROM_ALL)
message("Adding GLFW")
add_subdirectory(glfw EXCLUDE_FROM_ALL)
message("Adding GLM")
add_subdirectory(glm EXCLUDE_FROM_ALL)
message("Adding VMA")
add_subdirectory(VulkanMemoryAllocator EXCLUDE_FROM_ALL)

# ~~~
# IMGUI
if(GraphicsPlayground_Enable_ImGui)
  target_compile_definitions(GraphicsPlayground PUBLIC ENABLE_IMGUI)
  message("Adding ImGui")

  set(IMGUI_PATH "imgui")

  set(IMGUI_SOURCES
      "${IMGUI_PATH}/imgui.cpp" "${IMGUI_PATH}/imgui_demo.cpp"
      "${IMGUI_PATH}/imgui_draw.cpp" "${IMGUI_PATH}/imgui_tables.cpp"
      "${IMGUI_PATH}/imgui_widgets.cpp")

  set(IMGUI_HEADERS
      "${IMGUI_PATH}/imconfig.h" "${IMGUI_PATH}/imgui.h"
      "${IMGUI_PATH}/imgui_internal.h" "${IMGUI_PATH}/imstb_rectpack.h"
      "${IMGUI_PATH}/imstb_textedit.h" "${IMGUI_PATH}/imstb_truetype.h")

  # if GLFW
  set(IMGUI_SOURCES ${IMGUI_SOURCES}
                    "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp")
  set(IMGUI_HEADERS ${IMGUI_HEADERS} "${IMGUI_PATH}/backends/imgui_impl_glfw.h")

  # if Vulkan
  if(GraphicsPlayground_Graphics_Vulkan)
    set(IMGUI_SOURCES ${IMGUI_SOURCES}
                      "${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp")
    set(IMGUI_HEADERS ${IMGUI_HEADERS}
                      "${IMGUI_PATH}/backends/imgui_impl_vulkan.h")
  endif()

  target_sources(GraphicsPlayground PRIVATE ${IMGUI_SOURCES} ${IMGUI_HEADERS})
  target_include_directories(GraphicsPlayground PRIVATE ${IMGUI_PATH})
  # target_include_directories(GraphicsPlayground PRIVATE ${IMGUI_PATH}/imgui.h)
  target_include_directories(GraphicsPlayground PRIVATE ${IMGUI_PATH}/backends)
endif()

# ~~~
# Bullet3
message("Adding Bullet3")
# find_package(Bullet REQUIRED)
add_subdirectory(bullet3 EXCLUDE_FROM_ALL)
target_include_directories(GraphicsPlayground PRIVATE "bullet3/src")
# bullet3 adds this to the cache, makes it confusing with my use vr option
# unset(USE_OPENVR CACHE) set(Bullet_DIR "bullet3") if (DEFINED Bullet_DIR)
# message("Finding local Bullet") Find_Package(Bullet CONFIG)
# SET(BULLET_INCLUDE_DIR ${Bullet_DIR}/${BULLET_ROOT_DIR}/${BULLET_INCLUDE_DIR})
# SET(BLA ${Bullet_DIR}/${BULLET_ROOT_DIR}/${BULLET_LIBRARY_DIRS}) else()
# message("Finding system installed Bullet") Find_Package(Bullet) endif()

# ~~~
# STB

message("Adding stb")
set(STB_SOURCES "stb/stb_image.h")
target_sources(GraphicsPlayground PRIVATE ${STB_SOURCES})
target_include_directories(GraphicsPlayground PRIVATE "stb")

# ~~~
# Target links
message("Linking Targets")

target_include_directories(GraphicsPlayground
                           PRIVATE "VulkanMemoryAllocator/include")
target_include_directories(GraphicsPlayground PRIVATE "assimp/include")

target_link_libraries(
  GraphicsPlayground
  BulletCollision
  Bullet3Common
  BulletDynamics
  LinearMath
  assimp
  glfw
  glm
  VulkanMemoryAllocator
  ${OpenXR-Link})
