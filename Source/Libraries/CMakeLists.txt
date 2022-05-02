set(IMGUI_PATH "imgui")
set(STB_PATH "stb ")
set(VMA_PATH "VulkanMemoryAllocator/include ")

target_include_directories(GraphicsPlayground PRIVATE ${STB_PATH})
target_include_directories(GraphicsPlayground PRIVATE ${VMA_PATH})

set(IMGUI_SOURCES
    "${IMGUI_PATH}/imgui.cpp"
    "${IMGUI_PATH}/imgui_demo.cpp"
    "${IMGUI_PATH}/imgui_draw.cpp"
    "${IMGUI_PATH}/imgui_tables.cpp"
    "${IMGUI_PATH}/imgui_widgets.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp"
    CACHE INTERNAL ""
)

set(IMGUI_HEADERS
    "${IMGUI_PATH}/imconfig.h"
    "${IMGUI_PATH}/imgui.h"
    "${IMGUI_PATH}/imgui_internal.h"
    "${IMGUI_PATH}/imstb_rectpack.h"
    "${IMGUI_PATH}/imstb_textedit.h"
    "${IMGUI_PATH}/imstb_truetype.h"
    "${IMGUI_PATH}/backends/imgui_impl_glfw.h"
    "${IMGUI_PATH}/backends/imgui_impl_vulkan.h"
    CACHE INTERNAL ""
)

#Other

#add_subdirectory(assimp EXCLUDE_FROM_ALL)
add_subdirectory(glfw EXCLUDE_FROM_ALL)
add_subdirectory(glm EXCLUDE_FROM_ALL)
add_subdirectory(VulkanMemoryAllocator EXCLUDE_FROM_ALL)

#add_subdirectory(Libraries/googleTest EXCLUDE_FROM_ALL)
#set(BUILD_SHARED_LIBS OFF)
#add_subdirectory(Libraries/glslang EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/SPIRV-Headers EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/SPIRV-Tools EXCLUDE_FROM_ALL)
#add_subdirectory(Libraries/shaderc EXCLUDE_FROM_ALL)

set(EXTERNAL_INCLUDE_LIBRARIES ${EXTERNAL_INCLUDE_LIBRARIES} ${STB_PATH} ${VMA_PATH}
CACHE INTERNAL "")

set(EXTERNAL_LINK_LIBRARIES
${EXTERNAL_LINK_LIBRARIES} 
#assimp 
glfw 
glm
VulkanMemoryAllocator
CACHE INTERNAL "") 