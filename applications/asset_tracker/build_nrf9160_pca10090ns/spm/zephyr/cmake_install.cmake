# Install script for directory: C:/ncs.sojkk/zephyr

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
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/arch/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/lib/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/soc/arm/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/boards/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/ext/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/subsys/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/drivers/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/nrf/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/nffs/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/segger/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/mbedtls/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/mcuboot/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/mcumgr/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/tinycbor/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/nrfxlib/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/civetweb/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/fatfs/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/nordic/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/st/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/libmetal/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/lvgl/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/open-amp/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/openthread/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/modules/littlefs/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/kernel/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/cmake/flash/cmake_install.cmake")
  include("C:/ncs.sojkk/nrf/applications/asset_tracker/build_nrf9160_pca10090ns/spm/zephyr/cmake/reports/cmake_install.cmake")

endif()

