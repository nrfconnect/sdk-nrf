#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

find_package(Python3 REQUIRED)

function(lwm2m_carrier_divided_dfu)
  sysbuild_get(${DEFAULT_IMAGE}_image_dir IMAGE ${DEFAULT_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
  sysbuild_get(${DEFAULT_IMAGE}_kernel_name IMAGE ${DEFAULT_IMAGE} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)

  set(app_binary "${${DEFAULT_IMAGE}_image_dir}/zephyr/${${DEFAULT_IMAGE}_kernel_name}.signed.bin")
  set(output_dir "${CMAKE_BINARY_DIR}/lwm2m_carrier_divided_dfu")
  file(MAKE_DIRECTORY ${output_dir})

  add_custom_target(
    divided_dfu
    ALL
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/lib/bin/lwm2m_carrier/scripts/lwm2m_carrier_divided_dfu.py
    --input-file ${app_binary}
    --version-str ${SB_CONFIG_LWM2M_CARRIER_DIVIDED_DFU_VERSION}
    --max-file-size ${SB_CONFIG_LWM2M_CARRIER_DIVIDED_DFU_MAX_FILE_SIZE}
    --output-dir ${output_dir}
    DEPENDS
    ${app_binary}
    ${DEFAULT_IMAGE}_extra_byproducts
  )
endfunction(lwm2m_carrier_divided_dfu)

if(SYSBUILD)
  lwm2m_carrier_divided_dfu()
endif()
