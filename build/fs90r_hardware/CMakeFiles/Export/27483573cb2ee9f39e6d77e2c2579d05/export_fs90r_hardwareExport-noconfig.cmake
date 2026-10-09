#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "fs90r_hardware::fs90r_hardware" for configuration ""
set_property(TARGET fs90r_hardware::fs90r_hardware APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(fs90r_hardware::fs90r_hardware PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libfs90r_hardware.so"
  IMPORTED_SONAME_NOCONFIG "libfs90r_hardware.so"
  )

list(APPEND _cmake_import_check_targets fs90r_hardware::fs90r_hardware )
list(APPEND _cmake_import_check_files_for_fs90r_hardware::fs90r_hardware "${_IMPORT_PREFIX}/lib/libfs90r_hardware.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
