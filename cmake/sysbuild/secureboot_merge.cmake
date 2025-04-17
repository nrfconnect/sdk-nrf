#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(update_runner)
  cmake_parse_arguments(RUNNER "" "IMAGE;HEX" "" ${ARGN})

  check_arguments_required("update_runner" RUNNER IMAGE)
  check_arguments_required("update_runner" RUNNER HEX)

  get_target_property(bin_dir ${RUNNER_IMAGE} _EP_BINARY_DIR)
  set(runners_file ${bin_dir}/zephyr/runners.yaml)

  set(runners_content_update)
  file(STRINGS ${runners_file} runners_content)
  foreach(line IN LISTS runners_content)
    if("${line}" MATCHES "^.*hex_file: .*$")
      string(REGEX REPLACE "(.*hex_file:) .*" "\\1 ${RUNNER_HEX}" line ${line})
      set(${RUNNER_IMAGE}_NCS_RUNNER_HEX "${RUNNER_HEX}" CACHE INTERNAL
          "nRF Connect SDK NSIB controlled hex file"
      )
    endif()
    list(APPEND runners_content_update "${line}\n")
  endforeach()
  file(WRITE ${runners_file} ${runners_content_update})

  # NCS has updated the cache with an NCS_RUNNER file, thus re-create the sysbuild cache.
  # No need for CMAKE_RERUN in this case, as runners.yaml has been updated above.
  sysbuild_cache(CREATE APPLICATION ${RUNNER_IMAGE})
endfunction()

get_property(
  hex_files_to_merge
  GLOBAL PROPERTY
  NORDIC_SECURE_BOOT_HEX_FILES_TO_MERGE
)

sysbuild_get(BINARY_DIR IMAGE b0 VAR APPLICATION_BINARY_DIR CACHE)
sysbuild_get(BINARY_FILE IMAGE b0 VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
list(APPEND hex_files_to_merge ${BINARY_DIR}/zephyr/${BINARY_FILE}.hex)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  sysbuild_get(default_image_hex IMAGE ${DEFAULT_IMAGE} VAR BYPRODUCT_KERNEL_SIGNED_HEX_NAME CACHE)
  list(APPEND hex_files_to_merge ${default_image_hex})
endif()

add_custom_target(
  create_secure_boot_merged_hex
  ALL
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/mergehex.py
  -o ${CMAKE_BINARY_DIR}/merged.hex
  ${hex_files_to_merge}
  DEPENDS
  ${hex_files_to_merge}
)

update_runner(IMAGE ${DEFAULT_IMAGE} HEX ${CMAKE_BINARY_DIR}/merged.hex)
