cmake_minimum_required(VERSION 3.10)

project(engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(Stb REQUIRED)

file(GLOB_RECURSE SRC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


add_executable(engine ${SRC_SOURCES})

target_link_libraries(engine PRIVATE
    glfw
    glm::glm
    glad::glad
)

add_custom_command(
    TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:engine>/assets"
)