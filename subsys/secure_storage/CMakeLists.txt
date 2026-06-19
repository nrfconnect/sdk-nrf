# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

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
else()
  if(CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_PROVIDER_HUK_LIBRARY)
    list(APPEND ncs_secure_storage_src src/its_transform_aead_get_key_huk.c)
  endif()
endif() # CONFIG_SECURE_STORAGE_TRUSTED_STORAGE_COMPATIBILITY

if(ncs_secure_storage_src)
  zephyr_library()
  if(CONFIG_TF_PSA_CRYPTO_BUILTIN)
    # Inherit tfpsacrypto's private include directories for access to psa_crypto_driver_wrappers.h.
    zephyr_library_include_directories($<TARGET_PROPERTY:tfpsacrypto,INCLUDE_DIRECTORIES>)
  endif()
  zephyr_library_sources(${ncs_secure_storage_src})
endif()
