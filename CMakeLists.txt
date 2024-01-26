cmake_minimum_required(VERSION 3.26)
project(visualizer)

set(CMAKE_CXX_STANDARD 17)

find_package(ZeroMQ REQUIRED SHARED)

find_package(SFML 2.5 COMPONENTS system window graphics QUIET)
if(NOT SFML_FOUND)
    include(FetchContent)
    FetchContent_Declare(SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 2.6.x)
    FetchContent_MakeAvailable(SFML)
endif()
link_directories(${SFML_SOURCE_DIR})

add_executable(visualizer main.cpp cu.cpp
        config.h
        eventHandler.cpp
        eventHandler.h
        RenderBufferManager.cpp
        RenderBufferManager.h)
target_include_directories(visualizer PUBLIC "G:/Local/sainio/Documents/libzmq/out/install/include")
target_link_libraries(visualizer sfml-system sfml-window sfml-graphics "G:/Local/sainio/Documents/libzmq/out/install/lib/libzmq-mt-4_3_6.lib")
