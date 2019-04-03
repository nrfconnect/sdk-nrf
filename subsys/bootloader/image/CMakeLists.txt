#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(${CMAKE_CURRENT_LIST_DIR}/../include)
if (NOT DEFINED CONFIG_IS_SECURE_BOOTLOADER)
  add_subdirectory(bl_crypto_client)
endif()
set(NRF_BOOTLOADER_SCRIPTS ${NRF_DIR}/scripts/bootloader)
set(PROVISION_HEX_NAME     provision_data.hex)
set(PROVISION_HEX          ${PROJECT_BINARY_DIR}/${PROVISION_HEX_NAME})
set(SIGNED_KERNEL_HEX      ${CMAKE_BINARY_DIR}/signed_by_b0_${KERNEL_HEX_NAME})

set_property(
  GLOBAL APPEND PROPERTY
  HEX_FILES_TO_MERGE
  ${SIGNED_KERNEL_HEX}
  ${PROVISION_HEX}
)
set_property(
  GLOBAL APPEND PROPERTY
  HEX_FILES_TO_MERGE_TARGET
  signed_kernel_hex_target
  provision_target
)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/debug_keys.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/sign.cmake)

if(DEFINED DT_FLASH_AREA_S0_OFFSET)
  set(S0_ADDR ${DT_FLASH_AREA_S0_OFFSET})
else()
  set(S0_ADDR 0x8000)
endif()

if(DEFINED DT_FLASH_AREA_S1_OFFSET)
  set(S1_ADDR ${DT_FLASH_AREA_S1_OFFSET})
else()
  set(S1_ADDR 0xc000)
endif()

if(DEFINED DT_FLASH_AREA_PROVISION_OFFSET)
  set(PROVISION_ADDR ${DT_FLASH_AREA_PROVISION_OFFSET})
else()
  if(CONFIG_SOC_NRF51822_QFAA)
    set(PROVISION_ADDR 0x3f000)
  elseif(CONFIG_SOC_NRF51822_QFAB)
    set(PROVISION_ADDR 0x1f000)
  elseif(CONFIG_SOC_NRF51822_QFAB)
    set(PROVISION_ADDR 0x3f000)
  elseif(CONFIG_SOC_NRF52810)
    set(PROVISION_ADDR 0x2f000)
  elseif(CONFIG_SOC_NRF52832)
    set(PROVISION_ADDR 0x7f000)
  elseif(CONFIG_SOC_NRF9160)
    set(PROVISION_ADDR 0xFF8108) # UICR->OTP
  else()
    set(PROVISION_ADDR 0xff000)
  endif()
endif()

add_custom_command(
  OUTPUT
  ${PROVISION_HEX}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${NRF_BOOTLOADER_SCRIPTS}/provision.py
  --s0-addr ${S0_ADDR}
  --s1-addr ${S1_ADDR}
  --provision-addr ${PROVISION_ADDR}
  --public-key-files "${SIGNATURE_PUBLIC_KEY_FILE},${PUBLIC_KEY_FILES}"
  --output ${PROVISION_HEX}
  DEPENDS ${PROVISION_DEPENDS}
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMENT
  "Creating provision data for Bootloader, storing to ${PROVISION_HEX_NAME}"
  USES_TERMINAL
  )
add_custom_target(
  provision_target
  DEPENDS
  ${PROVISION_HEX}
  )

