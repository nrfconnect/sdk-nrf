#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING fault.c)

if (CONFIG_TFM_PARTITION_PLATFORM AND CONFIG_SOC_FAMILY_NORDIC_NRF)
  zephyr_library_named(tfm_api_nrf)

  # The non-secure API files are located in a folder associated with the TF-M
  # build system. Usually the cmake INSTALL mechanism is used to move these
  # API source files to be built by the zephyr build system.
  # For convenience we instead refer directly to the source files here.
  set(src_dir ${CMAKE_CURRENT_LIST_DIR}/tfm_boards/src)

  zephyr_library_sources(
    ${src_dir}/tfm_ioctl_ns_api.c
    )
endif()

if(CONFIG_TFM_USE_NS_APP)
  set_property(GLOBAL PROPERTY
    app_PM_HEX_FILE $<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>
    )
endif()

if (CONFIG_BOOTLOADER_MCUBOOT AND NOT CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY)
  # Configure the secondary partition to be non-secure
  if(SYSBUILD)
    function(mcuboot_single_check val)
      import_kconfig(CONFIG_ "${CMAKE_BINARY_DIR}/../mcuboot/zephyr/.config")
      set(${val} "${CONFIG_SINGLE_APPLICATION_SLOT}" PARENT_SCOPE)
    endfunction()

    set(mcuboot_single_slot)
    mcuboot_single_check(mcuboot_single_slot)

    if(mcuboot_single_slot)
      set_property(TARGET zephyr_property_target
        APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_NS_SECONDARY=n
      )
    else()
      set_property(TARGET zephyr_property_target
        APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_NS_SECONDARY=y
      )
    endif()
  else()
    set_property(TARGET zephyr_property_target
      APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DNRF_NS_SECONDARY=$<TARGET_PROPERTY:partition_manager,PM_mcuboot_secondary_IS_ENABLED>
    )
  endif()
endif()

if (CONFIG_TFM_HW_INIT_RESET_ON_BOOT)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_HW_INIT_RESET_ON_BOOT=ON
  )

  if (CONFIG_TFM_HW_INIT_NRF_PERIPHERALS)
    set_property(TARGET zephyr_property_target
      APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_HW_INIT_NRF_PERIPHERALS=ON
    )
  endif()
endif()

if (CONFIG_TFM_ALLOW_NON_SECURE_RESET)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_ALLOW_NON_SECURE_RESET=ON)
endif()

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_TFM_HALT_ON_CORE_PANIC=${CONFIG_TFM_HALT_ON_CORE_PANIC}
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_HW_UNIQUE_KEY=${CONFIG_HW_UNIQUE_KEY}
    -DCONFIG_HW_UNIQUE_KEY_RANDOM=${CONFIG_HW_UNIQUE_KEY_RANDOM}
    -DCRYPTO_TFM_BUILTIN_KEYS_DRIVER=${CONFIG_TFM_CRYPTO_BUILTIN_KEYS}
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DSECURE_UART1=${CONFIG_TFM_SECURE_UART}
)

if (CONFIG_TFM_SECURE_UART0)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_SECURE_UART_INSTANCE=0
  )
endif()

if (CONFIG_TFM_SECURE_UART1)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_SECURE_UART_INSTANCE=1
  )
endif()

if (CONFIG_TFM_SECURE_UART00)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_SECURE_UART_INSTANCE=00
  )
endif()

if (CONFIG_TFM_SECURE_UART20)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_SECURE_UART_INSTANCE=20
  )
endif()

if (CONFIG_TFM_SECURE_UART21)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_SECURE_UART_INSTANCE=21
  )
endif()

if (CONFIG_TFM_SECURE_UART22)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_SECURE_UART_INSTANCE=22
  )
endif()

if (CONFIG_TFM_SECURE_UART30)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_SECURE_UART_INSTANCE=30
  )
endif()

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_TFM_LOG_SHARE_UART=${CONFIG_TFM_SECURE_UART_SHARE_INSTANCE}
)

set_property(GLOBAL PROPERTY
  tfm_PM_HEX_FILE $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
  )

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DZEPHYR_NRF_MODULE_DIR=${ZEPHYR_NRF_MODULE_DIR}
  )

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DZEPHYR_BASE=${ZEPHYR_BASE}
  )

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DCRYPTO_HW_ACCELERATOR=True
  )


if (CONFIG_TFM_ALLOW_NON_SECURE_FAULT_HANDLING)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_ALLOW_NON_SECURE_FAULT_HANDLING=True
  )
endif()

if (CONFIG_TFM_LOG_LEVEL_SILENCE)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DPLATFORM_DEFAULT_UART_STDOUT=OFF
    )
endif()

if (CONFIG_TFM_PROFILE_TYPE_MINIMAL)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DPLATFORM_DEFAULT_ROTPK=OFF
    -DPLATFORM_DEFAULT_IAK=OFF
    -DPLATFORM_DEFAULT_NV_SEED=OFF
    -DPLATFORM_DEFAULT_OTP=OFF
    -DPLATFORM_DEFAULT_OTP_WRITEABLE=OFF
    -DPLATFORM_DEFAULT_NV_COUNTERS=OFF
    )
endif()

if (NOT CONFIG_MBEDTLS_PSA_CRYPTO_STORAGE_C)
  # Workaround: NCSDK-13530
  # Allow TF-M crypto to not depend on ITS when PSA crypto storage is disabled.
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCRYPTO_STORAGE_DISABLED=TRUE
    )
endif()

if (CONFIG_BOOTLOADER_MCUBOOT)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_BOOTLOADER_MCUBOOT=TRUE
    )
endif()

if (CONFIG_TFM_NRF_PROVISIONING)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DNRF_PROVISIONING=ON
    )

  message("
  TF-M Provisioning has been enabled
  The device must be correctly provisioned in order to boot.
  See TF-M: Provisioning image sample on how to provision the device for TF-M.
  ")
endif()

if (CONFIG_TFM_PSA_FRAMEWORK_HAS_MM_IOVEC)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DPSA_FRAMEWORK_HAS_MM_IOVEC=ON
    )
endif()

if (CONFIG_NFCT_PINS_AS_GPIOS OR CONFIG_TFM_NFCT_PINS_AS_GPIOS)
  set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_NFCT_PINS_AS_GPIOS=ON
  )
endif()

if (CONFIG_NRF_TRACE_PORT)
  set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_NRF_TRACE_PORT=${CONFIG_NRF_TRACE_PORT}
  )
endif()

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    # Pass Zephyr Python to TF-M so both uses identical Python.
    -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
)

# CONN_HANDLE_MAX_NUM is only needed if IPC mode is used
# The maximal number of secure services that are connected or requested at the same time
if (CONFIG_TFM_CONN_HANDLE_MAX_NUM)
  set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_TFM_DOORBELL_API=${CONFIG_TFM_CONN_HANDLE_MAX_NUM}
  )
endif()

if (CONFIG_NRF_APPROTECT_LOCK)
  set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_NRF_APPROTECT_LOCK=ON
  )
endif()

if (CONFIG_NRF_SECURE_APPROTECT_LOCK)
  set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_NRF_SECURE_APPROTECT_LOCK=ON
  )
endif()

zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/tfm)

# Default values from config_base.h in TF-M.
set(PLATFORM_SERVICE_INPUT_BUFFER_SIZE     ${CONFIG_TFM_PLATFORM_SERVICE_INPUT_BUFFER_SIZE})
set(PLATFORM_SERVICE_OUTPUT_BUFFER_SIZE    ${CONFIG_TFM_PLATFORM_SERVICE_OUTPUT_BUFFER_SIZE})
set(PLATFORM_SP_STACK_SIZE                 ${CONFIG_TFM_PLATFORM_SP_STACK_SIZE})
set(PLATFORM_NV_COUNTER_MODULE_DISABLED    ${CONFIG_TFM_PLATFORM_NV_COUNTER_MODULE_DISABLED})
set(CRYPTO_ENGINE_BUF_SIZE                 ${CONFIG_TFM_CRYPTO_ENGINE_BUF_SIZE})
set(CRYPTO_CONC_OPER_NUM                   ${CONFIG_TFM_CRYPTO_CONC_OPER_NUM})
set(CRYPTO_RNG_MODULE_ENABLED              ${CONFIG_TFM_CRYPTO_RNG_MODULE_ENABLED})
set(CRYPTO_KEY_MODULE_ENABLED              ${CONFIG_TFM_CRYPTO_KEY_MODULE_ENABLED})
set(CRYPTO_AEAD_MODULE_ENABLED             ${CONFIG_TFM_CRYPTO_AEAD_MODULE_ENABLED})
set(CRYPTO_MAC_MODULE_ENABLED              ${CONFIG_TFM_CRYPTO_MAC_MODULE_ENABLED})
set(CRYPTO_HASH_MODULE_ENABLED             ${CONFIG_TFM_CRYPTO_HASH_MODULE_ENABLED})
set(CRYPTO_CIPHER_MODULE_ENABLED           ${CONFIG_TFM_CRYPTO_CIPHER_MODULE_ENABLED})
set(CRYPTO_ASYM_SIGN_MODULE_ENABLED        ${CONFIG_TFM_CRYPTO_ASYM_SIGN_MODULE_ENABLED})
set(CRYPTO_ASYM_ENCRYPT_MODULE_ENABLED     ${CONFIG_TFM_CRYPTO_ASYM_ENCRYPT_MODULE_ENABLED})
set(CRYPTO_KEY_DERIVATION_MODULE_ENABLED   ${CONFIG_TFM_CRYPTO_KEY_DERIVATION_MODULE_ENABLED})
set(CRYPTO_PAKE_MODULE_ENABLED             ${CONFIG_TFM_CRYPTO_PAKE_MODULE_ENABLED})
set(CRYPTO_IOVEC_BUFFER_SIZE               ${CONFIG_TFM_CRYPTO_IOVEC_BUFFER_SIZE})
set(CRYPTO_NV_SEED                         ${CONFIG_TFM_CRYPTO_NV_SEED})
set(CRYPTO_SINGLE_PART_FUNCS_DISABLED      ${CONFIG_TFM_CRYPTO_SINGLE_PART_FUNCS_DISABLED})
set(CRYPTO_STACK_SIZE                      ${CONFIG_TFM_CRYPTO_PARTITION_STACK_SIZE})
set(TFM_FWU_BUF_SIZE                       0)
set(FWU_STACK_SIZE                         0)
set(ATTEST_INCLUDE_OPTIONAL_CLAIMS         ${CONFIG_TFM_ATTEST_INCLUDE_OPTIONAL_CLAIMS})
set(ATTEST_INCLUDE_COSE_KEY_ID             ${CONFIG_TFM_ATTEST_INCLUDE_COSE_KEY_ID})
set(ATTEST_STACK_SIZE                      ${CONFIG_TFM_ATTEST_STACK_SIZE})
set(ATTEST_TOKEN_PROFILE_PSA_IOT_1         ${CONFIG_TFM_ATTEST_TOKEN_PROFILE_PSA_IOT_1})
set(ATTEST_TOKEN_PROFILE_PSA_2_0_0         ${CONFIG_TFM_ATTEST_TOKEN_PROFILE_PSA_2_0_0})
set(ATTEST_TOKEN_PROFILE_ARM_CCA           ${CONFIG_TFM_ATTEST_TOKEN_PROFILE_ARM_CCA})
set(ITS_CREATE_FLASH_LAYOUT                ${CONFIG_TFM_ITS_CREATE_FLASH_LAYOUT})
set(ITS_RAM_FS                             ${CONFIG_TFM_TS_RAM_FS})
set(ITS_VALIDATE_METADATA_FROM_FLASH       ${CONFIG_TFM_ITS_VALIDATE_METADATA_FROM_FLASH})
set(ITS_MAX_ASSET_SIZE                     ${CONFIG_TFM_ITS_MAX_ASSET_SIZE})
set(ITS_BUF_SIZE                           ${CONFIG_TFM_ITS_BUF_SIZE})
set(ITS_NUM_ASSETS                         ${CONFIG_TFM_ITS_NUM_ASSETS})
set(ITS_STACK_SIZE                         ${CONFIG_TFM_ITS_STACK_SIZE})
set(PS_CREATE_FLASH_LAYOUT                 ${CONFIG_TFM_PS_CREATE_FLASH_LAYOUT})
set(PS_RAM_FS                              ${CONFIG_TFM_PS_RAM_FS})
set(PS_ROLLBACK_PROTECTION                 ${CONFIG_TFM_PS_ROLLBACK_PROTECTION})
set(PS_VALIDATE_METADATA_FROM_FLASH        ${CONFIG_TFM_PS_VALIDATE_METADATA_FROM_FLASH})
set(PS_MAX_ASSET_SIZE                      ${CONFIG_TFM_PS_MAX_ASSET_SIZE})
set(PS_NUM_ASSETS                          ${CONFIG_TFM_PS_NUM_ASSETS})
set(PS_STACK_SIZE                          ${CONFIG_TFM_PS_STACK_SIZE})
set(CONFIG_TFM_DOORBELL_API                ${CONFIG_TFM_DOORBELL_API})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tfm_config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/tfm_config.h)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DPROJECT_CONFIG_HEADER_FILE=${CMAKE_CURRENT_BINARY_DIR}/tfm_config.h
)

configure_file(${NRF_DIR}/subsys/nrf_security/configs/config_extra.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/config_extra.cmake
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DTFM_EXTRA_CONFIG_PATH="${CMAKE_CURRENT_BINARY_DIR}/config_extra.cmake"
)

if(SYSBUILD)
  set(BYPRODUCT_KERNEL_SIGNED_HEX_NAME "${CMAKE_BINARY_DIR}/zephyr/tfm_merged.hex"
      CACHE FILEPATH "Kernel hex file" FORCE
  )
endif()
