# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_SOC_NRF9251_GENERATE_BICR)
  set(bicr_json_file ${BOARD_DIR}/${SB_CONFIG_SOC_NRF9251_BICR_NAME})
  set(bicr_hex_file ${CMAKE_BINARY_DIR}/bicr.hex)
  set(bicrgen_script ${CMAKE_CURRENT_LIST_DIR}/bicr/bicrgen.py)
  set(svd_file ${ZEPHYR_HAL_NORDIC_MODULE_DIR}/nrfx/bsp/stable/mdk/nrf92/nrf9220/nrf9220_application.svd)

  set(app_bicr_json_file ${APP_DIR}/${SB_CONFIG_SOC_NRF9251_BICR_NAME})
  if(EXISTS ${app_bicr_json_file})
    set(bicr_json_file ${app_bicr_json_file})
  endif()

  if(EXISTS ${bicr_json_file})
    message(STATUS "Found BICR: ${bicr_json_file}")

    add_custom_command(
      OUTPUT ${bicr_hex_file}
      COMMAND
        ${Python3_EXECUTABLE}
        ${bicrgen_script}
        --svd ${svd_file}
        --input ${bicr_json_file}
        --output ${bicr_hex_file}
      DEPENDS
        ${bicrgen_script}
        ${bicr_json_file}
        ${svd_file}
      COMMENT "Generating BICR hex file: ${bicr_hex_file}"
      VERBATIM
    )
    add_custom_target(bicr ALL DEPENDS ${bicr_hex_file})
  else()
    message(WARNING "BICR file ${SB_CONFIG_SOC_NRF9251_BICR_NAME} not found in "
                    "${APP_DIR} or ${BOARD_DIR}, bicr.hex will not be generated. ")
  endif()
endif()
