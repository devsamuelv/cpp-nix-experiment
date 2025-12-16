cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 
      ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

project(teleop-control VERSION 0.1.0 LANGUAGES C CXX)
add_executable(teleop-control ${CMAKE_SOURCE_DIR}/src/main.cpp)

find_package(ament_cmake REQUIRED)
find_package(OpenCV REQUIRED imgcodecs imgproc videoio)
find_package(Torch REQUIRED)
find_package(rclcpp REQUIRED)
find_package(image_transport REQUIRED)
find_package(cv_bridge REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/src)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(src)
include_directories(src/helpers src/video)

target_link_libraries(teleop-control ${OpenCV_LIBS})
target_link_libraries(teleop-control ${TORCH_LIBRARIES})
target_link_libraries(teleop-control
  image_transport::image_transport
  rclcpp::rclcpp
  cv_bridge::cv_bridge
)

# Dont forget to add ament export include and dependcies calls from template 

include(CTest)
enable_testing()

install(TARGETS teleop-control DESTINATION bin)

# # problematic

# ament_export_include_directories(include)
# ament_export_dependencies(image_transport rosidl_default_runtime rclcpp sensor_msgs)

# if(BUILD_TESTING)
#   find_package(ament_lint_auto REQUIRED)
#   ament_lint_auto_find_test_dependencies()
# endif()

# ament_package()