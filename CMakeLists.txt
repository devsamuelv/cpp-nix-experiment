cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(teleop-control VERSION 0.1.0 LANGUAGES C CXX)

find_package(OpenCV REQUIRED)
find_package(Torch REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(teleop-control main.cpp)

target_link_libraries(teleop-control ${OpenCV_LIBS})
target_link_libraries(teleop-control ${TORCH_LIBRARIES})

include(CTest)
enable_testing()

install(TARGETS teleop-control DESTINATION bin)
