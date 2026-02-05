cmake_minimum_required(VERSION 3.20)
project(voxel-engine C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------

# Vulkan SDK path
set(VULKAN_SDK_PATH "$ENV{VULKAN_SDK}")
if(NOT VULKAN_SDK_PATH)
    set(VULKAN_SDK_PATH "$ENV{HOME}/VulkanSDK/1.4.335.1/macOS")
endif()

# Vulkan - uses VULKAN_SDK env var automatically, or system install
find_package(Vulkan REQUIRED)

# SDL3 - fetch if not found
find_package(SDL3 QUIET)
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        main
        GIT_SHALLOW    TRUE
    )
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL3)
endif()

# KTX - fetch if not found
find_package(Ktx QUIET)
if(NOT Ktx_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        ktx
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG        v4.3.2
        GIT_SHALLOW    TRUE
    )
    set(KTX_FEATURE_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
    set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
    set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(ktx)
endif()

# Slang - use pre-built binaries from libs/slang
# Download from: https://github.com/shader-slang/slang/releases
find_library(SLANG_LIBRARY 
    NAMES slang
    HINTS 
        "${CMAKE_SOURCE_DIR}/libs/slang/lib"
    NO_DEFAULT_PATH
)
find_path(SLANG_INCLUDE_DIR
    NAMES slang.h
    HINTS
        "${CMAKE_SOURCE_DIR}/libs/slang/include"
    NO_DEFAULT_PATH
)

if(SLANG_LIBRARY AND SLANG_INCLUDE_DIR)
    message(STATUS "Found Slang: ${SLANG_LIBRARY}")
    add_library(slang SHARED IMPORTED)
    set_target_properties(slang PROPERTIES
        IMPORTED_LOCATION "${SLANG_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}"
    )
    if(APPLE)
        # Fix rpath for macOS
        set_target_properties(slang PROPERTIES
            IMPORTED_SONAME "@rpath/libslang.dylib"
        )
    endif()
    set(SLANG_FOUND TRUE)
else()
    message(WARNING "Slang not found in libs/slang/")
    message(WARNING "Download from: https://github.com/shader-slang/slang/releases")
    set(SLANG_FOUND FALSE)
endif()

# --------------------------------------------------------------------------
# Source files
# --------------------------------------------------------------------------

set(SOURCES
    src/main.cpp
    core/platform/window.cpp
    gfx/vulkan/context.cpp
)

set(HEADERS
    core/platform/window.h
    gfx/vulkan/context.h
)

# --------------------------------------------------------------------------
# Executable
# --------------------------------------------------------------------------

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/libs
    ${Vulkan_INCLUDE_DIRS}
    "${VULKAN_SDK_PATH}/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3
    ktx
    $<$<BOOL:${SLANG_FOUND}>:slang>
)

# --------------------------------------------------------------------------
# Platform-specific settings
# --------------------------------------------------------------------------

if(APPLE)
    # MoltenVK setup for macOS
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
    
    # Point to MoltenVK ICD if VULKAN_SDK is set
    if(DEFINED ENV{VULKAN_SDK})
        set_property(TARGET ${PROJECT_NAME} PROPERTY
            XCODE_SCHEME_ENVIRONMENT
            "VK_ICD_FILENAMES=$ENV{VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json"
        )
    endif()
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dwmapi
    )
elseif(UNIX)
    # Linux - may need X11 or Wayland
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES})
    endif()
endif()

# --------------------------------------------------------------------------
# Build configurations
# --------------------------------------------------------------------------

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_BUILD)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zi /Od)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    endif()
else()
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
endif()

# --------------------------------------------------------------------------
# Shader compilation (optional - uncomment if using GLSL)
# --------------------------------------------------------------------------

# find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin)
# 
# set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
# set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
# file(MAKE_DIRECTORY ${SHADER_OUT_DIR})
# 
# file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp)
# 
# foreach(SHADER ${SHADERS})
#     get_filename_component(SHADER_NAME ${SHADER} NAME)
#     set(SHADER_OUT ${SHADER_OUT_DIR}/${SHADER_NAME}.spv)
#     add_custom_command(
#         OUTPUT ${SHADER_OUT}
#         COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUT}
#         DEPENDS ${SHADER}
#         COMMENT "Compiling ${SHADER_NAME}"
#     )
#     list(APPEND SPIRV_SHADERS ${SHADER_OUT})
# endforeach()
# 
# add_custom_target(shaders DEPENDS ${SPIRV_SHADERS})
# add_dependencies(${PROJECT_NAME} shaders)

# --------------------------------------------------------------------------
# Slang shader compilation (optional - uncomment to use)
# --------------------------------------------------------------------------

# set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
# set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
# file(MAKE_DIRECTORY ${SHADER_OUT_DIR})
# 
# file(GLOB SLANG_SHADERS ${SHADER_DIR}/*.slang)
# 
# foreach(SHADER ${SLANG_SHADERS})
#     get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
#     set(SHADER_OUT ${SHADER_OUT_DIR}/${SHADER_NAME}.spv)
#     add_custom_command(
#         OUTPUT ${SHADER_OUT}
#         COMMAND slangc ${SHADER} -profile glsl_450 -target spirv -o ${SHADER_OUT}
#         DEPENDS ${SHADER}
#         COMMENT "Compiling Slang shader: ${SHADER_NAME}"
#     )
#     list(APPEND SPIRV_SHADERS ${SHADER_OUT})
# endforeach()
# 
# add_custom_target(slang_shaders DEPENDS ${SPIRV_SHADERS})
# add_dependencies(${PROJECT_NAME} slang_shaders)
