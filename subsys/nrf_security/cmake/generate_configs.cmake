

# The term "Secure-only" means a system where TrustZone is unavailable/not enabled

# This macro is used for the following builds
# - NS interface built by TF-M (Results will will be installed in NCS)
macro(generate_mbedcrypto_interface_configs)
  # This must be set TODO:FILLME
  if(NOT PSA_CRYPTO_CONFIG_INTERFACE_PATH)
      message(FATAL_ERROR "generate_mbedcrypto_interface_configs expects PSA_CRYPTO_CONFIG_INTERFACE_PATH to be set")
  endif()

  if(NOT CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
    message("=========== Generating psa_crypto_config ===============")

    # Make a copy of the configurations changed for the PSA interface
    kconfig_backup_current_config(CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
    kconfig_backup_current_config(CONFIG_MBEDTLS_PSA_CRYPTO_C)
    kconfig_backup_current_config(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
    kconfig_backup_current_config(CONFIG_MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)
    kconfig_backup_current_config(CONFIG_MBEDTLS_THREADING)
    kconfig_backup_current_config(CONFIG_MBEDTLS_THREADING_ALT)

    message("=========== Checkpoint: backup ===============")

    # Set generated_include_path which is used by the config-generation
    set(generated_include_path ${PSA_CRYPTO_CONFIG_INTERFACE_PATH})

    # CONFIG_MBEDTLS_PSA_CRYPTO_SPM must be set to false for interface builds (Relevant for check_config.h)
    set(CONFIG_MBEDTLS_PSA_CRYPTO_SPM False)
    # CONFIG_MBEDTLS_PSA_CRYPTO_C must be set to false for interface builds (Relevant for check_config.h)
    set(CONFIG_MBEDTLS_PSA_CRYPTO_C False)
    # CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER must be turned off because the TF-M interface
    # uses psa_key_id_t for the interface and not mbedtls_svc_key_id_t
    set(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER False)
    # CONFIG_MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT must be turned of because the NS world doesn't use it
    set(CONFIG_MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT False)
    if(CONFIG_BUILD_WITH_TFM)
      # Disable threading for the PSA interface used in TF-M build (NS and S image)
      set(CONFIG_MBEDTLS_THREADING_C False)
      set(CONFIG_MBEDTLS_THREADING_ALT False)
    endif()

    # Empty out previous versions of interface config-files
    file(REMOVE_RECURSE ${generated_include_path})

    # Generate MBEDCRYPTO_CONFIG_FILE 
    if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C)
      include(${NRF_SECURITY_ROOT}/cmake/legacy_crypto_config.cmake)
    else()
      include(${NRF_SECURITY_ROOT}/cmake/nrf_config.cmake)
    endif()

    # Generate the PSA_CRYPTO_CONFIG_FILE (PSA_WANT_XXXX configurations)
    include(${NRF_SECURITY_ROOT}/cmake/psa_crypto_want_config.cmake)

    # Note: Interface doesn't need PSA_CRYPTO_USER_CONFIG_FILE

    # Restore the backup configurations 
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PSA_CRYPTO_C)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_THREADING)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_THREADING_ALT)

    message("=========== End psa_crypto_config ===============")
  endif()
endmacro()


# This macro is only used for the following builds
# - TF-M crypto partition builds (inside the secure image)
# - Secure-only builds
macro(generate_mbedcrypto_library_configs)
  if(NOT PSA_CRYPTO_CONFIG_LIBRARY_PATH)
      message(FATAL_ERROR "generate_mbedcrypto_library_configs expects PSA_CRYPTO_CONFIG_LIBRARY_PATH to be set")
  endif()

  if(NOT (CONFIG_MBEDTLS_PSA_CRYPTO_SPM OR CONFIG_PSA_SSF_CRYPTO_CLIENT))

    message("=========== Generating psa_crypto_library_config ===============")

    # Make a backup of the configurations changed for the PSA library
    kconfig_backup_current_config(CONFIG_MBEDTLS_PSA_CRYPTO_C)
    kconfig_backup_current_config(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
    kconfig_backup_current_config(CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
    kconfig_backup_current_config(CONFIG_MBEDTLS_USE_PSA_CRYPTO)
    kconfig_backup_current_config(CONFIG_MBEDTLS_PLATFORM_PRINTF_ALT)
    kconfig_backup_current_config(CONFIG_MBEDTLS_THREADING)
    kconfig_backup_current_config(CONFIG_MBEDTLS_THREADING_ALT)


    message("=========== Checkpoint: backup ===============")

    # Set generated_include_path which is used by the config-generation files
    set(generated_include_path ${PSA_CRYPTO_CONFIG_LIBRARY_PATH})

    # Empty out previous versions of library config-files
    file(REMOVE_RECURSE ${generated_include_path})

    # CONFIG_MBEDTLS_PSA_CRYPTO_C must be set as to true for all library builds
    set(CONFIG_MBEDTLS_PSA_CRYPTO_C True)

    # Handle configurations required by library build inside TF-M (NS world doesn't use psa_crypto_library_config)
    if(CONFIG_BUILD_WITH_TFM)
      set(MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER True)
      # CONFIG_MBEDTLS_PSA_CRYPTO_SPM must be set for the library build in TF-M
      set(CONFIG_MBEDTLS_PSA_CRYPTO_SPM True)
      # CONFIG_MBEDTLS_USE_PSA_CRYPTO must be unset for library build in TF-M
      set(CONFIG_MBEDTLS_USE_PSA_CRYPTO False)
      # CONFIG_MBEDTLS_PLATFORM_PRINTF_ALT must be set for the library build in TF-M
      set(CONFIG_MBEDTLS_PLATFORM_PRINTF_ALT True)
      # Disable threading for the PSA interface used in TF-M build (NS and S image)
      set(CONFIG_MBEDTLS_THREADING_C False)
      set(CONFIG_MBEDTLS_THREADING_ALT False)
    endif()

    # Generate MBEDCRYPTO_CONFIG_FILE
    if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C)
      include(${NRF_SECURITY_ROOT}/cmake/legacy_crypto_config.cmake)
    else()
      include(${NRF_SECURITY_ROOT}/cmake/nrf_config.cmake)
    endif()

    # Generate the PSA_CRYPTO_CONFIG_FILE (PSA_WANT_XXXX configurations)
    include(${NRF_SECURITY_ROOT}/cmake/psa_crypto_want_config.cmake)

    # Generate the PSA_CRYPTO_USER_CONFIG_FILE (PSA_NEED configurations)
    include(${NRF_SECURITY_ROOT}/cmake/psa_crypto_config.cmake)

    # Restore the backup configurations 
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PSA_CRYPTO_C)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_USE_PSA_CRYPTO)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_PLATFORM_PRINTF_ALT)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_THREADING)
    kconfig_restore_backup_config(CONFIG_MBEDTLS_THREADING_ALT)

    message("=========== End psa_crypto_library_config ===============")
  endif()
endmacro()
