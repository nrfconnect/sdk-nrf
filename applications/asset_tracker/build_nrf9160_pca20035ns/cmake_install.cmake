# Install script for directory: C:/ncs.sojkk/nrf/applications/asset_tracker

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "ZRelease")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/src/orientation_detector/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/src/ui/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/src/cloud_codec/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/src/gps_controller/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/src/env_sensors/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/src/light_sensor/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
