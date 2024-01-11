cmake_minimum_required(VERSION 3.26)
project(visualizer)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

add_executable(visualizer main.cpp
        cu.cpp)
target_link_libraries(visualizer ${OpenCV_LIBS} sfml-system sfml-window sfml-graphics)
