cmake_minimum_required(VERSION 3.5.0)
project(PV2i VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")


set(OpenCV_DIR ./lib/opencv)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# PV2i
add_executable(PV2i src/PV2i.cpp)
target_link_libraries(PV2i ${OpenCV_LIBS})
target_link_libraries(PV2i -static-libgcc -static-libstdc++)

# test_ORB
add_executable(TestORB test/test_ORB.cpp)
target_link_libraries(TestORB ${OpenCV_LIBS})
target_link_libraries(TestORB -static-libgcc -static-libstdc++)


