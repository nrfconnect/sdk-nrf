#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Use the single source of truth for TF-M options (Nordic overlay)
include(${ZEPHYR_NRF_MODULE_DIR}/modules/trusted-firmware-m/cmake/nrf_tfm_apply_options.cmake)

# Set the necessary configurations for TF-M build
nrf_tfm_append_cmake_options(
  -DNRF_SECURITY_SETTINGS=\"ZEPHYR_DOTCONFIG=${DOTCONFIG}
                            GCC_M_CPU=${GCC_M_CPU}
                            ARM_MBEDTLS_PATH=${ARM_MBEDTLS_PATH}
                            ZEPHYR_AUTOCONF=${AUTOCONF_H}\"
)

# Set the path used by TF-M to build mbedcrypto in the secure image
nrf_tfm_set_mbedcrypto_path(${CMAKE_CURRENT_LIST_DIR}/../tfm)

if(CONFIG_TFM_ITS_ENCRYPTED)
  nrf_tfm_append_cmake_options(-DITS_ENCRYPTION=1)
endif()
