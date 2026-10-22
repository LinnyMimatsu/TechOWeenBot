cmake_minimum_required(VERSION 3.8)
project(ds3225_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

find_path(LGPIO_INCLUDE_DIR lgpio.h REQUIRED)
find_library(LGPIO_LIBRARY lgpio REQUIRED)

add_executable(ds3225_gpio_node src/ds3225_gpio_node.cpp)
target_include_directories(ds3225_gpio_node PRIVATE ${LGPIO_INCLUDE_DIR})
target_link_libraries(ds3225_gpio_node ${LGPIO_LIBRARY})
ament_target_dependencies(ds3225_gpio_node rclcpp sensor_msgs std_msgs)

add_executable(ds3225_serial_bridge src/ds3225_serial_bridge.cpp)
ament_target_dependencies(ds3225_serial_bridge rclcpp sensor_msgs std_msgs)

install(TARGETS
  ds3225_gpio_node
  ds3225_serial_bridge
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  config
  launch
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY
  firmware/ds3225_test
  DESTINATION share/${PROJECT_NAME}/firmware
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
