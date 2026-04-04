cmake_minimum_required(VERSION 3.14)
project(engine CXX C)

set(CMAKE_CXX_STANDARD 17)
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

file(GLOB_RECURSE SRC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(engine ${SRC_SOURCES})

# Strict error checking for both platforms, warnings are treated as errors.
if(MSVC)
    target_compile_options(engine PRIVATE /W4 /permissive- /WX)
else()
    target_compile_options(engine PRIVATE 
        -Wall -Wextra -Wpedantic -pedantic-errors -Werror
        -Wvla 
        -Wconversion
        -Wfloat-conversion
        -Wsign-conversion) 
endif()


target_include_directories(engine PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(engine PRIVATE
    glfw
    glm::glm
    glad::glad
    imgui::imgui
)

add_custom_command(
    TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:engine>/assets"
)