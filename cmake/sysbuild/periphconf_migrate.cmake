# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# This file includes extra sysbuild POST_CMAKE logic to assist with
# autogenerating an IronSide SE compatible UICR.
#
# It is enabled when SB_CONFIG_NRF_PERIPHCONF_MIGRATE=y is set and
# one of the sysbuild images has CONFIG_NRF_HALTIUM_GENERATE_UICR=y.
# It uses nrf-regtool to produce an extra source file for that image
# (periphconf_migrated.c) based on multiple image devicetrees.
#
# NOTE: This is a temporary solution for NCS. The planned solution
# for upstream Zephyr will not require nrf-regtool.

set(ironside_uicr_images "")
set(ironside_uicr_main_image)
foreach(image ${POST_CMAKE_IMAGES})
  sysbuild_get(${image}_54h20_app IMAGE ${image} VAR CONFIG_SOC_NRF54H20_CPUAPP KCONFIG)
  sysbuild_get(${image}_54h20_rad IMAGE ${image} VAR CONFIG_SOC_NRF54H20_CPURAD KCONFIG)
  if(${image}_54h20_app OR ${image}_54h20_rad)
    list(APPEND ironside_uicr_images ${image})
    if(NOT DEFINED ironside_uicr_main_image)
      sysbuild_get(image_generates_uicr IMAGE ${image} VAR CONFIG_NRF_HALTIUM_GENERATE_UICR KCONFIG)
      if(image_generates_uicr)
        set(ironside_uicr_main_image ${image})
      endif()
    endif()
  endif()
endforeach()
if(DEFINED ironside_uicr_main_image)
  find_program(NRF_REGTOOL nrf-regtool REQUIRED)
  set(nrf_regtool_cmd
    COMMAND
    ${CMAKE_COMMAND} -E env PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
    ${NRF_REGTOOL}
  )
  ExternalProject_Get_Property(${ironside_uicr_main_image} BINARY_DIR)
  set(nrf_regtool_migrate_cmd
    ${nrf_regtool_cmd}
    uicr-migrate
    --edt-pickle-file ${BINARY_DIR}/zephyr/edt.pickle
    --output-periphconf-file ${APPLICATION_BINARY_DIR}/periphconf_migrated.c
  )
  foreach(image ${ironside_uicr_images})
    ExternalProject_Get_Property(${image} BINARY_DIR)
    sysbuild_get(image_soc IMAGE ${image} VAR CONFIG_SOC KCONFIG)
    set(image_uicr_hex ${PROJECT_BINARY_DIR}/uicr_${image}.hex)
    message(STATUS "Running nrf-regtool uicr-compile for ${image}")
    execute_process(
      ${nrf_regtool_cmd}
      uicr-compile
      --edt-pickle-file ${BINARY_DIR}/zephyr/edt.pickle
      --product-name ${image_soc}
      --output-file ${image_uicr_hex}
      WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
      COMMAND_ERROR_IS_FATAL ANY
    )
    list(APPEND nrf_regtool_migrate_cmd --uicr-hex-file ${image_uicr_hex})
  endforeach()
  message(STATUS "Running nrf-regtool uicr-migrate for ${ironside_uicr_main_image}")
  execute_process(
    ${nrf_regtool_migrate_cmd}
    WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
  )
endif()
