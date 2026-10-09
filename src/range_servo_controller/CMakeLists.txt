
cmake_minimum_required(VERSION 3.8)
project(range_servo_controller)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(range_servo_controller
  src/range_servo_controller.cpp
)

ament_target_dependencies(range_servo_controller
  rclcpp
  sensor_msgs
  std_msgs
)

install(TARGETS
  range_servo_controller
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()