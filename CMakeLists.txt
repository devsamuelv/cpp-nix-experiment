cmake_minimum_required(VERSION 3.10.0)
project(teleop-control VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenCV REQUIRED)
find_package(GTK2 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(teleop-control main.cpp)

target_link_libraries(teleop-control ${OpenCV_LIBS} )

include(CTest)
enable_testing()

install(TARGETS teleop-control DESTINATION bin)
