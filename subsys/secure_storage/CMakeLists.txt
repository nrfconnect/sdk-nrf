# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS AND CONFIG_PARTITION_MANAGER_ENABLED)
  message(FATAL_ERROR "
    CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS is
    not supported when partition manager is enabled.
    ")
endif()

zephyr_library()
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)

if(CONFIG_SECURE_STORAGE_TRUSTED_STORAGE_COMPATIBILITY)
  add_subdirectory(compatibility)
else()
  zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_HUK_LIBRARY
    src/its_transform_aead_get_key_huk.c
  )
endif()
