# Install script for directory: C:/ncs.sojkk/nrf/lib

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
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/modules/nrf/lib/bin/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/modules/nrf/lib/bsdlib/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/modules/nrf/lib/dk_buttons_and_leds/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/modules/nrf/lib/at_notif/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/modules/nrf/lib/at_host/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/modules/nrf/lib/at_cmd_parser/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca20035ns/zephyr/modules/nrf/lib/modem_info/cmake_install.cmake")

endif()

