cmake_minimum_required(VERSION 3.5.0)
project(PV2i VERSION 2.1.1 LANGUAGES C CXX)
add_definitions(-DPROGRAM_VERSION="${PROJECT_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_BUILD_TYPE "Debug")

# set(CMAKE_BUILD_TYPE "Release")
MESSAGE("*** Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/release")

# OpenCV
set(OpenCV_DIR ./ThirdParty/opencv)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# PV2i
add_executable(PV2i
    src/PV2i.cpp
    src/ArgumentParser.cpp
    src/RunOptions.cpp
)
target_link_libraries(PV2i ${OpenCV_LIBS})

# test_ORB
add_executable(TestORB test/test_ORB.cpp)
target_link_libraries(TestORB ${OpenCV_LIBS})
