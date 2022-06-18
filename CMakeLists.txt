cmake_minimum_required(VERSION 3.21.0)
message("----  Starting ----")
project(GraphicsPlayground VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ~~~
# Get Platform
set(PLATFORM_WINDOWS 0)
set(PLATFORM_APPLE 0)
set(PLATFORM_LINUX 0)

if(WIN32)
  set(PLATFORM_WINDOWS 1)
endif()
if(APPLE)
  set(PLATFORM_APPLE 1)
endif()
if(UNIX AND NOT APPLE) # for Linux, BSD, Solaris, Minix
  set(PLATFORM_LINUX 1)
endif()

message(STATUS ${CMAKE_GENERATOR})

set(WORK_DIR_FOLDER_REL "WorkDir")
get_filename_component(WORK_DIR_FOLDER
                       "${PROJECT_BINARY_DIR}/Source/" ABSOLUTE)
message("BUILD_OUTPUT ${WORK_DIR_FOLDER}")
message("CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
message("PROJECT_BINARY_DIR ${PROJECT_BINARY_DIR}")

# ~~~
# make project
add_subdirectory(Source)
add_subdirectory(Shaders)

# ~~~
# Apply Platform to project
target_compile_definitions(GraphicsPlayground
                           PUBLIC PLATFORM_NAME=${CMAKE_SYSTEM_NAME})
target_compile_definitions(GraphicsPlayground
                           PUBLIC PLATFORM_WINDOWS=${PLATFORM_WINDOWS})
target_compile_definitions(GraphicsPlayground
                           PUBLIC PLATFORM_APPLE=${PLATFORM_APPLE})
target_compile_definitions(GraphicsPlayground
                           PUBLIC PLATFORM_LINUX=${PLATFORM_LINUX})

message(STATUS "setting WORK_DIR_REL to \"${WORK_DIR_FOLDER_REL}\"")
target_compile_definitions(GraphicsPlayground
                            PUBLIC WORK_DIR_REL=\"${WORK_DIR_FOLDER_REL}\")