cmake_minimum_required(VERSION 3.5.0)
project(PV2i VERSION 0.1.0 LANGUAGES C CXX)

add_executable(PV2i src/PV2i.cpp)

set(OpenCV_DIR ./lib/opencv)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(PV2i ${OpenCV_LIBS})

