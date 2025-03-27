cmake_minimum_required(VERSION 3.10.0)
project(teleop-control VERSION 0.1.0 LANGUAGES C CXX)

add_executable(teleop-control main.cpp)

include(CTest)
enable_testing()

install(TARGETS teleop-control DESTINATION bin)
