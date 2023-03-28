# Copyright 2020-2023 AVSystem <avsystem@avsystem.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(anjay_zephyr_demo)
set(root_dir ${ANJAY_ZEPHYR_CLIENT_DIR})

if(CONFIG_PARTITION_MANAGER_ENABLED)
    # defined in nrf/cmake/partition_manager.cmake
    if(NOT EXISTS "${static_configuration_file}")
        set(pm_missing_message "Missing static partition manager file for board: ${BOARD}")

        if(ACTIVE_BOARD_REVISION)
            set(pm_missing_message "${pm_missing_message}, rev: ${ACTIVE_BOARD_REVISION}")
        endif()

        if(CONF_FILE_BUILD_TYPE)
            set(pm_missing_message "${pm_missing_message}, build type: ${CONF_FILE_BUILD_TYPE}")
        endif()

        message(FATAL_ERROR "${pm_missing_message}")
    endif()
elseif(CONFIG_ANJAY_CLIENT_BUILD_MCUBOOT_AUTOMATICALLY)
    if("${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}" STREQUAL "")
        if(NOT CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE)
            message(FATAL_ERROR "Either MCUBOOT_SIGNATURE_KEY_FILE or MCUBOOT_GENERATE_UNSIGNED_IMAGE need to be set in Kconfig")
        endif()
    else()
        set(MCUBOOT_SIGNATURE_KEY_FILE "${CONFIG_MCUBOOT_SIGNATURE_KEY_FILE}")
        if(NOT IS_ABSOLUTE "${MCUBOOT_SIGNATURE_KEY_FILE}")
            set(MCUBOOT_SIGNATURE_KEY_FILE "${WEST_TOPDIR}/${MCUBOOT_SIGNATURE_KEY_FILE}")
        endif()
    endif()
    make_directory("${CMAKE_CURRENT_BINARY_DIR}/mcuboot")
    execute_process(COMMAND "${CMAKE_COMMAND}"
                            -GNinja
                            "-DAPPLICATION_CONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}/child_image/mcuboot"
                            "-DBOARD=${BOARD}"
                            "-DCONF_FILE=${ZEPHYR_BASE}/../bootloader/mcuboot/boot/zephyr/prj.conf"
                            "-DCONF_FILE_INCLUDE_FRAGMENTS=ON"
                            "-DCONFIG_BOOT_SIGNATURE_KEY_FILE=\"${MCUBOOT_SIGNATURE_KEY_FILE}\""
                            "-DDTC_OVERLAY_FILE=${CMAKE_CURRENT_SOURCE_DIR}/child_image/mcuboot/boards/${BOARD}.overlay"
                            "${ZEPHYR_BASE}/../bootloader/mcuboot/boot/zephyr"
                    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/mcuboot"
                    RESULT_VARIABLE MCUBOOT_CMAKE_RESULT)
    if(NOT MCUBOOT_CMAKE_RESULT EQUAL 0)
        message(FATAL_ERROR "Configuring MCUboot failed")
    endif()

    add_custom_target(mcuboot COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_CURRENT_BINARY_DIR}/mcuboot")

    # NOTE: This is largely copied from CMakeLists.txt in Zephyr itself.
    # However, using that would require setting HEX_FILES_TO_MERGE *before*
    # find_package(Zephyr), and we can't do that because we need to examine
    # Kconfig variables, which are populated by find_package(Zephyr).
    # That's why we do it ourselves here...
    set(MERGED_HEX_NAME "${CMAKE_CURRENT_BINARY_DIR}/zephyr/merged.hex")
    # In Zephyr 3.2, mergehex.py has been moved
    # from scripts/mergehex.py to scripts/build/mergehex.py
    set(MERGEHEX_SCRIPT "${ZEPHYR_BASE}/scripts/build/mergehex.py")
    if(NOT EXISTS "${MERGEHEX_SCRIPT}")
        set(MERGEHEX_SCRIPT "${ZEPHYR_BASE}/scripts/mergehex.py")
    endif()
    add_custom_command(OUTPUT "${MERGED_HEX_NAME}"
                       COMMAND "${PYTHON_EXECUTABLE}"
                               "${MERGEHEX_SCRIPT}"
                               -o "${MERGED_HEX_NAME}"
                               "${CMAKE_CURRENT_BINARY_DIR}/mcuboot/zephyr/zephyr.hex"
                               "${CMAKE_CURRENT_BINARY_DIR}/zephyr/zephyr.signed.hex"
                       DEPENDS mcuboot zephyr_final)
    add_custom_target(mergehex ALL DEPENDS "${MERGED_HEX_NAME}")

    if(TARGET runners_yaml_props_target)
        get_target_property(RUNNERS_YAML_CONTENTS runners_yaml_props_target yaml_contents)
        string(REGEX REPLACE "zephyr[a-zA-Z0-9._-]*[.]hex" "merged.hex" RUNNERS_YAML_CONTENTS "${RUNNERS_YAML_CONTENTS}")
        set_property(TARGET runners_yaml_props_target PROPERTY yaml_contents "${RUNNERS_YAML_CONTENTS}")
    endif()
endif()

if(CONFIG_ANJAY_ZEPHYR_FACTORY_PROVISIONING_INITIAL_FLASH)
    set(app_sources
        src/factory_provisioning/factory_flash.c
        src/factory_provisioning/factory_flash.h
        src/factory_provisioning/provisioning_app.c)
else()
    set(app_sources
        src/main_app.c
        src/sensors_config.c
        src/sensors_config.h
        src/status_led.c
        src/status_led.h
        src/peripherals.h)
endif()

target_sources(app PRIVATE
               ${app_sources})
