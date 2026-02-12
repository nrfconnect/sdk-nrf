#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Single source of truth for appending TF-M CMake options and setting
# TFM_MBEDCRYPTO_PATH. Both the Nordic TF-M module (nrf/modules/trusted-firmware-m)
# and nrf_security (config_to_tf-m.cmake) must use these functions instead of
# calling set_property on zephyr_property_target directly. This avoids scattered
# TFM_CMAKE_OPTIONS and makes the dependency explicit.
#

# Append options to TFM_CMAKE_OPTIONS (passed to TF-M when it configures).
# Usage: nrf_tfm_append_cmake_options(-DVAR=value -DOTHER=value ...)
function(nrf_tfm_append_cmake_options)
  if(ARGN)
    set_property(TARGET zephyr_property_target
      APPEND PROPERTY TFM_CMAKE_OPTIONS
      ${ARGN}
    )
  endif()
endfunction()

# Set the path used by TF-M to build mbedcrypto in the secure image.
# Usage: nrf_tfm_set_mbedcrypto_path(path)
function(nrf_tfm_set_mbedcrypto_path path)
  set_property(TARGET zephyr_property_target
    PROPERTY TFM_MBEDCRYPTO_PATH ${path}
  )
endfunction()
