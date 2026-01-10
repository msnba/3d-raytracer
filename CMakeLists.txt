cmake_minimum_required(VERSION 3.10)

project(engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_executable(engine bin/main.cpp bin/shader.cpp)

target_link_libraries(engine PRIVATE
    glfw
    glm::glm
    glad::glad
)