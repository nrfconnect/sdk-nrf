#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Set the necessary configurations for TF-M build
set_property(TARGET zephyr_property_target
  APPEND
    PROPERTY TFM_CMAKE_OPTIONS
      -DNRF_SECURITY_SETTINGS=\"ZEPHYR_DOTCONFIG=${DOTCONFIG}
                                GCC_M_CPU=${GCC_M_CPU}
                                ARM_MBEDTLS_PATH=${ARM_MBEDTLS_PATH}
                                ZEPHYR_AUTOCONF=${AUTOCONF_H}\"
)

# Set the path used by TF-M to build mbedcrypto in the secure image
# The CMakelists.txt in this folder is called when building mbed TLS
set_property(TARGET zephyr_property_target
  PROPERTY TFM_MBEDCRYPTO_PATH ${CMAKE_CURRENT_LIST_DIR}/../tfm
)

# Set the configuration files
set_property(TARGET zephyr_property_target
  APPEND
    PROPERTY TFM_CMAKE_OPTIONS
      -DTFM_MBEDCRYPTO_CONFIG_PATH:STRING=${CONFIG_MBEDTLS_CFG_FILE}
      -DTFM_MBEDCRYPTO_PLATFORM_EXTRA_CONFIG_PATH:STRING=${CONFIG_MBEDTLS_USER_CONFIG_FILE}
)

if(CONFIG_TFM_BL2)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DMCUBOOT_MBEDCRYPTO_CONFIG_FILEPATH:STRING=${CONFIG_MBEDTLS_CFG_FILE}
  )
endif()

if(CONFIG_PSA_ITS_ENCRYPTED)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DTFM_ITS_ENCRYPTED=1
  )
endif()
