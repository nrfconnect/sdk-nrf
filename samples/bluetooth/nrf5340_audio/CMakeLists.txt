#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if (CONFIG_AUDIO_DFU EQUAL 1)
    set(mcuboot_OVERLAY_CONFIG
    ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-mcuboot.conf
    )

endif()

if (CONFIG_AUDIO_DFU EQUAL 2)
    set(mcuboot_OVERLAY_CONFIG
        ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-mcuboot_external_flash.conf
    )
    set(mcuboot_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-dfu_external_flash.overlay)
    set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-dfu_external_flash.overlay)
    set(mcuboot_PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/pm_dfu_external_flash.yml)
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/pm_dfu_external_flash.yml)
endif()

if ((CONFIG_AUDIO_DFU EQUAL 1) OR (CONFIG_AUDIO_DFU EQUAL 2))
    set(hci_rpmsg_b0n_OVERLAY_CONFIG ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/b0n.conf)
    if (CONFIG_B0N_MINIMAL)
        list(APPEND hci_rpmsg_b0n_OVERLAY_CONFIG overlay-minimal-size.conf)
    endif()
endif()

# Flag which defines whether application is compiled as gateway/dongle or headset
add_compile_definitions(HEADSET=1)
add_compile_definitions(GATEWAY=2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(NRF5340_AUDIO)

string(TIMESTAMP NRF5340_AUDIO_CORE_APP_COMP_DATE "%a %b %d %H:%M:%S %Y")

# Generate fw_info_app.c
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/utils/fw_info_app.c.in"
               "${CMAKE_BINARY_DIR}/fw_info_app.c"
               @ONLY)

# Target sources below are specific to the nRF5340 Audio DK HW
target_sources(app PRIVATE
    src/main.c
    ${CMAKE_BINARY_DIR}/fw_info_app.c
    )

# Include application events and configuration headers
zephyr_library_include_directories(
    src/audio
    src/bluetooth
    src/drivers
    src/events
    src/modules
    src/utils
    src/utils/macros
)

zephyr_library_include_directories(app PRIVATE
    ${ZEPHYR_NRF_MODULE_DIR}/boards/arm/nrf5340_audio_dk_nrf5340)

# Application sources
add_subdirectory(src/audio)
add_subdirectory(src/bluetooth)
add_subdirectory(src/drivers)
add_subdirectory(src/events)
add_subdirectory(src/modules)
add_subdirectory(src/utils)

## Cirrus Logic
if (CONFIG_HW_CODEC_CIRRUS_LOGIC)
    if (ZEPHYR_CIRRUS_LOGIC_MODULE_DIR)
        add_subdirectory(${ZEPHYR_CIRRUS_LOGIC_MODULE_DIR} cirrus_logic_bin_dir)
    else()
        message(FATAL_ERROR "Cirrus Logic/sdk-mcu-drivers repository not found\n")
    endif()
endif()
