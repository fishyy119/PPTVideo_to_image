cmake_minimum_required(VERSION 3.5.0)
project(PV2i VERSION 0.1.0 LANGUAGES C CXX)

add_executable(PV2i PV2i.cpp)

# set(OpenCV_DIR  D:/OpenCV/opencv-4.4.0/new_build/)
set(OpenCV_DIR D:/OpenCV/opencv-4.4.0/new_build/install)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(PV2i ${OpenCV_LIBS})

