# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD)
  message(FATAL_ERROR "
    CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD cannot be used
    when CONFIG_SECURE_STORAGE_TRUSTED_STORAGE_COMPATIBILITY is enabled.")
endif()

zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_SETTINGS
  src/its_store_settings_get.c
)
zephyr_library_sources_ifdef(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_TSBC
  src/its_transform_tsbc.c
)
