#
# Copyright (c) 2018 - 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

################################################################################

# The application uses the configuration/<board> scheme for configuration files.
set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_LIST_DIR}/configuration/\${NORMALIZED_BOARD_TARGET}")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("nRF Desktop"
        VERSION 0.1)

################################################################################

assert(CONFIG_DESKTOP_HID_REPORT_DESC "HID report descriptor file must be specified")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  ${CONFIG_DESKTOP_HID_REPORT_DESC}
  )
# NORDIC SDK APP END

# Include application events and configuration headers
zephyr_library_include_directories(
  src/events
  src/util
  )

zephyr_include_directories(
  configuration/common
  ${APPLICATION_CONFIG_DIR}
  )

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/hw_interface)
add_subdirectory(src/modules)
add_subdirectory(src/util)

if(CONFIG_IMG_MANAGER)
  zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
endif()

if(CONFIG_BOOTLOADER_MCUBOOT)
  if(NOT (CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP OR
          CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP OR
          CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD OR
          CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_USING_MOVE))
    message(WARNING "
            ------------------------------------------------------------------------------
            --- WARNING: The currently selected mode of the MCUboot bootloader has not ---
            --- been used or tested with any nRF Desktop configuration.                ---
            ------------------------------------------------------------------------------
            ")
  endif()

  if(CONFIG_MCUBOOT_BOOTLOADER_MODE_RAM_LOAD)
    message(WARNING "
            -----------------------------------------------------------------------------
            --- WARNING: The RAM load mode of the MCUboot bootloader is experimental. ---
            -----------------------------------------------------------------------------
            ")

    # Enforce the specific RAM layout in DTS when MCUboot uses the RAM load mode.
    dt_nodelabel(app_rxm_region_node NODELABEL cpuapp_sram_app_rxm_region)
    if(NOT app_rxm_region_node)
      message(FATAL_ERROR
              "Missing RXM (ROM + RAM) region definition (cpuapp_sram_app_rxm_region) for the "
              "application image in DTS.")
    endif()

    dt_nodelabel(mcuboot_ram_region_node NODELABEL cpuapp_sram_mcuboot_ram_region)
    if(NOT mcuboot_ram_region_node)
      message(FATAL_ERROR
              "Missing RAM region definition (cpuapp_sram_mcuboot_ram_region) for the MCUboot "
              "bootloader in DTS.")
    endif()

    dt_reg_addr(app_rxm_region_addr PATH ${app_rxm_region_node})
    dt_reg_addr(mcuboot_ram_region_addr PATH ${mcuboot_ram_region_node})
    if(mcuboot_ram_region_addr LESS_EQUAL app_rxm_region_addr)
      message(FATAL_ERROR
              "The start address of the RXM region (0x${app_rxm_region_addr}) must be located "
              "before the start address of the MCUboot RAM region (0x${mcuboot_ram_region_addr})."
              "The current order is wrong — modify your DTS RAM layout to fix this error.")
    endif()

    # Ensure that generated application image will fit in the defined RAM partition after build.
    zephyr_linker_sources(RODATA linker/mcuboot_ram_load.ld)
  endif()
endif()

if(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)
  zephyr_linker_sources(SECTIONS nrf_desktop.ld)
  zephyr_linker_section(NAME config_channel_modules KVMA RAM_REGION GROUP RODATA_REGION NOINPUT)
  zephyr_linker_section_configure(SECTION config_channel_modules
                                  INPUT "config_channel_modules"
                                  SYMBOLS __start_config_channel_modules __stop_config_channel_modules
                                  KEEP SORT NAME)
endif()
