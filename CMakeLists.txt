cmake_minimum_required(VERSION 3.26)
project(visualizer)

set(CMAKE_CXX_STANDARD 17)

find_package(ZeroMQ REQUIRED)

find_package(SFML 2.5 COMPONENTS system window graphics QUIET)
if(NOT SFML_FOUND)
    include(FetchContent)
    FetchContent_Declare(SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 2.6.x)
    FetchContent_MakeAvailable(SFML)
endif()
link_directories(${SFML_SOURCE_DIR})

add_executable(visualizer main.cpp cu.cpp)
target_link_libraries(visualizer sfml-system sfml-window sfml-graphics zmq)
