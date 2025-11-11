# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_sources(
  soc.c
)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF7120_PREENG
  soc_preeng.c
)
zephyr_include_directories(.)
add_subdirectory(${ZEPHYR_BASE}/soc/nordic/common ${CMAKE_BINARY_DIR}/common)

# Ensure that image size aligns with 16 bytes so that MRAMC finalizes all writes
# for the image correctly
zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end align.ld)

# We need a buffer in memory in a static location which can be used by
# the KMU peripheral. The KMU has a static destination address,for nRF7120
# this address is defined as 0x200F_FF00, which is the last 256 Bytes in the SRAM.
if(NOT CONFIG_BUILD_WITH_TFM AND CONFIG_PSA_NEED_CRACEN_KMU_DRIVER)
  # Exclamation mark is printable character with the lowest number in ASCII table.
  # We are sure that this file will be included first.
  zephyr_linker_sources(RAM_SECTIONS SORT_KEY ! kmu_push_area_section.ld)
endif()

if(CONFIG_NRF_WIFI_WICR_GENERATE_HEX)
  set(WICR_GEN_SCRIPT ${ZEPHYR_NRF_MODULE_DIR}/soc/nordic/nrf71/wicr/gen_wicr.py)
  set(WICR_VALIDATE_SCRIPT ${ZEPHYR_NRF_MODULE_DIR}/soc/nordic/nrf71/wicr/validate_wicr.py)
  set(WICR_HEX_OUTPUT ${ZEPHYR_BINARY_DIR}/wicr.hex)
  set(ZEPHYR_DTS_FILE ${ZEPHYR_BINARY_DIR}/zephyr.dts)
  set(ZEPHYR_HEX_FILE ${ZEPHYR_BINARY_DIR}/zephyr.hex)

  if(CMAKE_VERBOSE_MAKEFILE)
    set(WICR_ARGS_VERBOSE "--verbose")
  else()
    set(WICR_ARGS_VERBOSE)
  endif()

  # Validate WICR and IPC memory consistency after DTS is compiled
  add_custom_target(wicr_validate
    COMMAND ${PYTHON_EXECUTABLE} ${WICR_VALIDATE_SCRIPT}
      --dts ${ZEPHYR_DTS_FILE}
      ${WICR_ARGS_VERBOSE}
    DEPENDS ${ZEPHYR_DTS_FILE}
    WORKING_DIRECTORY ${ZEPHYR_BINARY_DIR}
    COMMENT "Validating WICR and IPC memory consistency"
    VERBATIM
  )

  # Generate wicr.hex after DTS is validated and zephyr.hex is built
  add_custom_command(
    OUTPUT ${WICR_HEX_OUTPUT}
    COMMAND ${PYTHON_EXECUTABLE} ${WICR_GEN_SCRIPT}
      --node wicr
      --dts ${ZEPHYR_DTS_FILE}
      --output ${WICR_HEX_OUTPUT}
      ${WICR_ARGS_VERBOSE}
    DEPENDS wicr_validate ${ZEPHYR_DTS_FILE} ${ZEPHYR_HEX_FILE}
    WORKING_DIRECTORY ${ZEPHYR_BINARY_DIR}
    COMMENT "Generating WICR hex file"
    VERBATIM
  )

  # Create a custom target that depends on wicr.hex
  add_custom_target(wicr_hex ALL DEPENDS ${WICR_HEX_OUTPUT})

  # Ensure validation runs before generating wicr.hex
  add_dependencies(wicr_hex wicr_validate)

  # Merge wicr.hex with the final zephyr hex file
  set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE
    ${ZEPHYR_HEX_FILE}
    ${WICR_HEX_OUTPUT}
  )
endif()
