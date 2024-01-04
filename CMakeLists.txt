cmake_minimum_required(VERSION 3.26)
project(visualizer)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(visualizer main.cpp)
target_link_libraries(visualizer ${OpenCV_LIBS})
