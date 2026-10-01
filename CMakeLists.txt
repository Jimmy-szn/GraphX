cmake_minimum_required(VERSION 3.10)
project(GraphX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB SOURCES "src/*.cpp" "src/*.c")

add_executable(GraphX ${SOURCES})
target_include_directories( GraphX PUBLIC
 src/
 vendor/
)
target_link_libraries(
    GraphX PRIVATE
    OpenGL::GL
    glfw
)