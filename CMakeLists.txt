cmake_minimum_required(VERSION 3.14)

if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
endif()

project(engine CXX C)

# Compiler guard
if(MSVC)
    message(STATUS "Compiler recognized as MSVC")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Compiler recognized as GCC")
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Please use MSVC or GCC.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

file(GLOB_RECURSE SRC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
list(FILTER SRC_SOURCES EXCLUDE REGEX "tiny_obj_impl\\.cpp")

# Strict error checking for both platforms, warnings are treated as errors.
if(MSVC)
    add_executable(engine WIN32 ${SRC_SOURCES})
    target_compile_options(engine PRIVATE /W4 /permissive- /WX
        $<$<CONFIG:Debug>:/MTd>
        $<$<NOT:$<CONFIG:Debug>>:/MT>
    )
    target_link_options(engine PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
else()
    add_executable(engine ${SRC_SOURCES})
    target_compile_options(engine PRIVATE
        -Wall -Wextra -pedantic-errors -Werror
        -Wvla
        -Wconversion
        -Wfloat-conversion
        -Wsign-conversion)
endif()

find_path(PORTABLE_FILE_DIALOGS_INCLUDE_DIRS "portable-file-dialogs.h")
target_include_directories(engine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/include"
    "${Stb_INCLUDE_DIR}"
    ${PORTABLE_FILE_DIALOGS_INCLUDE_DIRS}
)

target_link_libraries(engine PRIVATE
    glfw
    glm::glm
    glad::glad
    imgui::imgui
    tinyobjloader::tinyobjloader

)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    assets/raytracer.comp
    assets/pass.vert
    assets/pass.frag
)

file(READ "assets/raytracer.comp" RAYTRACER_COMP)
file(READ "assets/pass.vert" PASS_VERT)
file(READ "assets/pass.frag" PASS_FRAG)
configure_file(assets/shader_sources.h.in include/shader_sources.h)

add_custom_command(
    TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:engine>/assets"
)