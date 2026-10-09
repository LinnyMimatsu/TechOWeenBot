#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "ros2_servo_motion::ros2_servo_motion" for configuration ""
set_property(TARGET ros2_servo_motion::ros2_servo_motion APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(ros2_servo_motion::ros2_servo_motion PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libros2_servo_motion.so"
  IMPORTED_SONAME_NOCONFIG "libros2_servo_motion.so"
  )

list(APPEND _cmake_import_check_targets ros2_servo_motion::ros2_servo_motion )
list(APPEND _cmake_import_check_files_for_ros2_servo_motion::ros2_servo_motion "${_IMPORT_PREFIX}/lib/libros2_servo_motion.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
