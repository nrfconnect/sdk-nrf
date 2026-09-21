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
include(common/common.cmake)
include(cracenpsa/cracenpsa.cmake)

# The KMU pushes keys to a fixed destination address, so that area has to be
# reserved for it. Every SoC with a CRACEN KMU reserves it with a kmu_push_area
# node in its devicetree. Warn when it is missing, since the destination address
# is written into the provisioned KMU slot metadata and cannot be corrected
# afterwards. TF-M reserves the area in its own linker script.
if(NOT BUILD_INSIDE_TFM AND NOT CONFIG_BUILD_WITH_TFM AND CONFIG_PSA_NEED_CRACEN_KMU_DRIVER)
  dt_nodelabel(kmu_push_area_node NODELABEL kmu_push_area)
  if(NOT kmu_push_area_node)
    message(WARNING
      "The CRACEN KMU driver is enabled but the devicetree has no kmu_push_area "
      "node reserving the KMU push area. The KMU will push keys to an address "
      "that nothing has reserved. If this board redefines the RAM layout, leave "
      "the push area from the SoC devicetree in place.")
  endif()
endif()

if((CONFIG_PSA_NEED_CRACEN_CTR_SIZE_WORKAROUNDS AND
    (CONFIG_PSA_NEED_CRACEN_CTR_AES OR CONFIG_PSA_NEED_CRACEN_CCM_AES)) OR
   CONFIG_PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
  include(cracen_sw/cracen_sw.cmake)
endif()

# Add include-directories
target_include_directories(psa_crypto_library_config
  INTERFACE
    ${cracen_driver_include_dirs}
)

add_library(cracen_psa_driver STATIC
  ${cracen_driver_sources}
)

target_include_directories(cracen_psa_driver
  PRIVATE
    ${cracen_driver_internal_include_dirs}
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
    nrf_security_utils
)

# Give access to cmsis, nrfx and MDK inside TF-M build
# to allow for referencing nRF events.
if(BUILD_INSIDE_TFM)
  target_link_libraries(cracen_psa_driver
    PRIVATE
      platform_s
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
