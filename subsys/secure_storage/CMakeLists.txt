# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS AND CONFIG_PARTITION_MANAGER_ENABLED)
  message(FATAL_ERROR "
    CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS is
    not supported when partition manager is enabled.
    ")
endif()

if(CONFIG_SECURE_STORAGE_TRUSTED_STORAGE_COMPATIBILITY)

  if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD)
    message(FATAL_ERROR "
      CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_AEAD cannot be used
      when CONFIG_SECURE_STORAGE_TRUSTED_STORAGE_COMPATIBILITY is enabled.")
  endif()

  if(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_SETTINGS)
    list(APPEND ncs_secure_storage_src compatibility/src/its_store_settings_get.c)
  endif()

  if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_IMPLEMENTATION_TSBC)
    list(APPEND ncs_secure_storage_src compatibility/src/its_transform_tsbc.c)
  endif()

endif() # CONFIG_SECURE_STORAGE_TRUSTED_STORAGE_COMPATIBILITY

if(ncs_secure_storage_src)
  zephyr_library()
  zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
  zephyr_library_sources(${ncs_secure_storage_src})
endif()
