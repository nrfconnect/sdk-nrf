#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Clear out the cracen_driver_sources before source additions
set(cracen_driver_sources)

# Add more CRACEN sources and include dirs from the ext directory
include(sxsymcrypt/sxsymcrypt.cmake)
include(silexpk/silexpk.cmake)
include(cracenpsa/cracenpsa.cmake)
if(CONFIG_PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS OR
   (CONFIG_SOC_NRF54LV10A AND CONFIG_PSA_NEED_CRACEN_CTR_AES))
  include(cracen_sw/cracen_sw.cmake)
endif()

# Add include-directories
target_include_directories(psa_crypto_library_config
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/include/
    ${cracen_driver_include_dirs}
)

add_library(cracen_psa_driver STATIC
  ${cracen_driver_sources}
)

# The CRACEN sources have not been written to comply with this gcc
# warning
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(cracen_psa_driver
    PRIVATE
      -Wno-pointer-sign
  )
endif()

target_link_libraries(cracen_psa_driver
  PRIVATE
    psa_crypto_library_config
    psa_interface
    mbedcrypto_base
    nrf_security_utils
)

# Give access to cmsis, nrfx and MDK inside TF-M build
# to allow for referencing nRF events. Note that
# tfm_sp_log needs to be added because __assert.h requires
# it
if(BUILD_INSIDE_TFM)
  target_link_libraries(cracen_psa_driver
    PRIVATE
      platform_s
      tfm_sp_log
  )
else()
  # Ensure zephyr/offsets.h is generated as this is a dependency for CRACEN driver
  # But not when building OpenThread because this casues a cyclic dependency
  add_dependencies(cracen_psa_driver zephyr_generated_headers)
endif()

# Link psa_core with this PSA crypto driver
target_link_libraries(psa_core
  PRIVATE
    cracen_psa_driver
)

nrf_security_add_zephyr_options_library(cracen_psa_driver)
