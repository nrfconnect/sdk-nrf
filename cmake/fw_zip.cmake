#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(generate_dfu_zip)
  set(oneValueArgs OUTPUT TYPE TARGET IMAGE)
  set(multiValueArgs BIN_FILES SCRIPT_PARAMS DEPENDS)
  cmake_parse_arguments(GENZIP "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (NOT(
    GENZIP_BIN_FILES AND
    GENZIP_SCRIPT_PARAMS AND
    GENZIP_OUTPUT AND
    GENZIP_TYPE
    ))
    message(FATAL_ERROR "Missing required param")
  endif()

  if(SYSBUILD AND GENZIP_IMAGE)
    set(APPNAME ${GENZIP_IMAGE})
    sysbuild_get(CONFIG_BUILD_OUTPUT_META IMAGE ${GENZIP_IMAGE} VAR CONFIG_BUILD_OUTPUT_META KCONFIG)
    sysbuild_get(CONFIG_BOARD IMAGE ${GENZIP_IMAGE} VAR CONFIG_BOARD KCONFIG)
    sysbuild_get(CONFIG_SOC IMAGE ${GENZIP_IMAGE} VAR CONFIG_SOC KCONFIG)

    if(CONFIG_BUILD_OUTPUT_META)
      sysbuild_get(application_image_dir IMAGE ${GENZIP_IMAGE} VAR APPLICATION_BINARY_DIR CACHE)
      sysbuild_get(CONFIG_KERNEL_BIN_NAME IMAGE ${GENZIP_IMAGE} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
      set(meta_info_file ${GENZIP_IMAGE}_extra_byproducts)
      set(meta_argument --meta-info-file ${application_image_dir}/zephyr/${CONFIG_KERNEL_BIN_NAME}.meta)
    endif()
  else()
    get_filename_component(APPNAME ${APPLICATION_SOURCE_DIR} NAME)
    if(CONFIG_BUILD_OUTPUT_META)
      set(meta_info_file ${PROJECT_BINARY_DIR}/${KERNEL_META_NAME})
      set(meta_argument --meta-info-file ${meta_info_file})
    endif()
  endif()

  add_custom_command(
    OUTPUT ${GENZIP_OUTPUT}
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/generate_zip.py
    --bin-files ${GENZIP_BIN_FILES}
    --output ${GENZIP_OUTPUT}
    --name "${APPNAME}"
    ${meta_argument}
    ${GENZIP_SCRIPT_PARAMS}
    "type=${GENZIP_TYPE}"
    "board=${CONFIG_BOARD}"
    "soc=${CONFIG_SOC}"
    DEPENDS ${GENZIP_DEPENDS} ${GENZIP_BIN_FILES} ${meta_info_file}
    )

  get_filename_component(TARGET_NAME ${GENZIP_OUTPUT} NAME)
  string(REPLACE "." "_" TARGET_NAME ${TARGET_NAME})

  add_custom_target(
    ${TARGET_NAME}
    ALL
    DEPENDS ${GENZIP_OUTPUT}
    )
endfunction()
