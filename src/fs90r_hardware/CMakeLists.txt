
cmake_minimum_required(VERSION 3.8)
project(fs90r_hardware)

if(CMAKE_COMPILER_IS_GNUCXX OR
   CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(THIS_PACKAGE_INCLUDE_DEPENDS
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
)

find_package(ament_cmake REQUIRED)

foreach(Dependency IN ITEMS ${THIS_PACKAGE_INCLUDE_DEPENDS})
  find_package(${Dependency} REQUIRED)
endforeach()

find_path(LGPIO_INCLUDE_DIR lgpio.h REQUIRED)
find_library(LGPIO_LIBRARY lgpio REQUIRED)

add_library(fs90r_hardware SHARED
  src/fs90r_system.cpp
  src/fs90r_servo.cpp
)

target_compile_features(fs90r_hardware PUBLIC cxx_std_17)

target_include_directories(fs90r_hardware PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${LGPIO_INCLUDE_DIR}
)

target_link_libraries(fs90r_hardware PUBLIC
  ${LGPIO_LIBRARY}
)

ament_target_dependencies(fs90r_hardware PUBLIC
  ${THIS_PACKAGE_INCLUDE_DEPENDS}
)

pluginlib_export_plugin_description_file(
  hardware_interface fs90r_hardware.xml
)

install(DIRECTORY include/ DESTINATION include)
install(FILES fs90r_hardware.xml DESTINATION share/${PROJECT_NAME})
install(DIRECTORY config urdf DESTINATION share/${PROJECT_NAME})
install(DIRECTORY config urdf launch DESTINATION share/${PROJECT_NAME})

install(TARGETS fs90r_hardware
  EXPORT export_fs90r_hardware
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

ament_export_targets(export_fs90r_hardware HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})

ament_package()