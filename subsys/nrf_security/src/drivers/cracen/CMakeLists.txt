#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Clear out the cracen_driver_sources before source additions
set(cracen_driver_sources)

# Add more CRACEN sources and include dirs from the ext directory
include(${NRF_SECURITY_ROOT}/src/utils/nrf_security_utils.cmake)
include(sxsymcrypt/sxsymcrypt.cmake)
include(silexpk/silexpk.cmake)
include(sicrypto/sicrypto.cmake)
include(cracenpsa/cracenpsa.cmake)

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
target_compile_options(cracen_psa_driver
  PRIVATE
    -Wno-pointer-sign
    -Wno-unused-function
)

target_link_libraries(cracen_psa_driver
  PRIVATE
    psa_crypto_library_config
    psa_interface
)

# Link oberon_psa_core with this PSA crypto driver
target_link_libraries(oberon_psa_core
  PRIVATE
    cracen_psa_driver
)

nrf_security_add_zephyr_options(cracen_psa_driver)
