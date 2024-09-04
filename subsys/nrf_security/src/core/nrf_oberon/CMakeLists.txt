#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_CORE_PATH}/library/
  platform.c
  platform_util.c
  )

append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_CORE_PATH}/library/
  psa_crypto.c
  psa_crypto_client.c
  psa_crypto_slot_management.c
  psa_crypto_storage.c
)

add_library(oberon_psa_core STATIC
    ${src_crypto_core_oberon}
)

# Add the nordic version of psa_crypto_driver_wrappers with the core (out of tree)
target_sources(oberon_psa_core
  PRIVATE
    ${NRF_SECURITY_ROOT}/src/psa_crypto_driver_wrappers.c
)

target_link_libraries(oberon_psa_core
  PRIVATE
    psa_crypto_library_config
    psa_interface
)

# Disable warnings showing up in Oberon PSA crypto code.
target_compile_options(oberon_psa_core
  PRIVATE
    -Wno-stringop-overflow
    -Wno-stringop-overread
    -Wno-strict-aliasing
)

target_link_libraries(oberon_psa_core
  PRIVATE
    mbedcrypto_base
)

nrf_security_add_zephyr_options(oberon_psa_core)

target_link_libraries(${mbedcrypto_target}
  PRIVATE
    oberon_psa_core
)

