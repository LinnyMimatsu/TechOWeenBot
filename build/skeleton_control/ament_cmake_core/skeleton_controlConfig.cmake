# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_skeleton_control_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED skeleton_control_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(skeleton_control_FOUND FALSE)
  elseif(NOT skeleton_control_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(skeleton_control_FOUND FALSE)
  endif()
  return()
endif()
set(_skeleton_control_CONFIG_INCLUDED TRUE)

# output package information
if(NOT skeleton_control_FIND_QUIETLY)
  message(STATUS "Found skeleton_control: 0.0.0 (${skeleton_control_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'skeleton_control' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT skeleton_control_DEPRECATED_QUIET)
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(skeleton_control_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "")
foreach(_extra ${_extras})
  include("${skeleton_control_DIR}/${_extra}")
endforeach()
