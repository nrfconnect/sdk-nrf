#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (NOT CONFIG_TFM_BUILD_NS AND CONFIG_TFM_PARTITION_PLATFORM AND CONFIG_SOC_FAMILY_NRF)
  zephyr_library_named(tfm_api_nrf)

  # The non-secure API files are located in a folder associated with the TF-M
  # build system. Usually the cmake INSTALL mechanism is used to move these
  # API source files to be built by the zephyr build system.
  # For convenience we instead refer directly to the source files here.
  set(src_dir ${CMAKE_CURRENT_LIST_DIR}/../tfm/boards/src)

  zephyr_library_sources(
    ${src_dir}/tfm_ioctl_ns_api.c
    )
endif()

if (CONFIG_TFM_BL2)
  # Add the mcuboot hex file built by TFM as the contents of the bl2 partitions,
  # so it can be flashed together with the app/tfm hex files.
  set_property(GLOBAL PROPERTY
    bl2_PM_HEX_FILE ${CMAKE_BINARY_DIR}/tfm/bin/bl2.hex
    )
  if (CONFIG_TFM_REGRESSION_NS)
    set_property(GLOBAL PROPERTY
      tfm_nonsecure_PM_HEX_FILE ${CMAKE_BINARY_DIR}/tfm_ns_signed.hex
      )
  else()
    set_property(GLOBAL PROPERTY
      tfm_nonsecure_PM_HEX_FILE ${CMAKE_BINARY_DIR}/zephyr_ns_signed.hex
      )
  endif()
  set_property(GLOBAL PROPERTY
    tfm_secure_PM_HEX_FILE ${CMAKE_BINARY_DIR}/tfm_s_signed.hex
    )
else()
  if (CONFIG_TFM_USE_NS_APP)
    set_property(GLOBAL PROPERTY
      app_PM_HEX_FILE $<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>
      )
  endif()
endif()

if (CONFIG_BOOTLOADER_MCUBOOT AND NOT CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY)
  # Configure the secondary partition to be non-secure
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
	-DNRF_NS_SECONDARY=$<TARGET_PROPERTY:partition_manager,PM_mcuboot_secondary_IS_ENABLED>
  )
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

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_TFM_LOG_SHARE_UART=${CONFIG_TFM_SECURE_UART_SHARE_INSTANCE}
)

set_property(GLOBAL PROPERTY
  tfm_PM_HEX_FILE $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
  )

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_DIR=${ZEPHYR_NRF_MODULE_DIR}
  )

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DZEPHYR_BASE=${ZEPHYR_BASE}
  )

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS -DCRYPTO_HW_ACCELERATOR=True
  )

if (CONFIG_TFM_LOG_LEVEL_SILENCE)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DPLATFORM_DEFAULT_UART_STDOUT=OFF
    )
endif()

if (CONFIG_TFM_PROFILE_TYPE_MINIMAL)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DPLATFORM_DEFAULT_ATTEST_HAL=OFF
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
endif()

if (CONFIG_NFCT_PINS_AS_GPIOS)
  set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCONFIG_NFCT_PINS_AS_GPIOS=${CONFIG_NFCT_PINS_AS_GPIOS}
  )
endif()

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    # Pass Zephyr Python to TF-M so both uses identical Python.
    -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
    # TF-M calls back into nrf_security which loads Zephyr Python lookup
    # mechanism, thus also set PYTHON_PREFER.
    -DPYTHON_PREFER=${Python3_EXECUTABLE}
)

zephyr_include_directories(${NRF_DIR}/include/tfm)

# Default values from config_base.h in TF-M.
set(PLATFORM_SERVICE_INPUT_BUFFER_SIZE     64)
set(PLATFORM_SERVICE_OUTPUT_BUFFER_SIZE    64)
set(PLATFORM_SP_STACK_SIZE                 0x500)
set(PLATFORM_NV_COUNTER_MODULE_DISABLED    ${CONFIG_TFM_PROFILE_TYPE_MINIMAL})
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
set(CRYPTO_IOVEC_BUFFER_SIZE               ${CONFIG_TFM_CRYPTO_IOVEC_BUFFER_SIZE})
set(CRYPTO_NV_SEED                         ${CONFIG_TFM_CRYPTO_NV_SEED})
set(CRYPTO_SINGLE_PART_FUNCS_DISABLED      0)
set(CRYPTO_STACK_SIZE                      ${CONFIG_TFM_CRYPTO_PARTITION_STACK_SIZE})
set(TFM_FWU_BUF_SIZE                       PSA_FWU_MAX_WRITE_SIZE)
set(FWU_STACK_SIZE                         0x600)
set(ATTEST_INCLUDE_OPTIONAL_CLAIMS         1)
set(ATTEST_INCLUDE_COSE_KEY_ID             0)
set(ATTEST_STACK_SIZE                      0x700)
set(ATTEST_TOKEN_PROFILE_PSA_IOT_1         1)
set(ITS_CREATE_FLASH_LAYOUT                1)
set(ITS_RAM_FS                             0)
set(ITS_VALIDATE_METADATA_FROM_FLASH       1)
set(ITS_MAX_ASSET_SIZE                     512)
set(ITS_BUF_SIZE                           ITS_MAX_ASSET_SIZE)
set(ITS_NUM_ASSETS                         10)
set(ITS_STACK_SIZE                         0x720)
set(PS_CREATE_FLASH_LAYOUT                 1)
set(PS_RAM_FS                              0)
set(PS_ROLLBACK_PROTECTION                 1)
set(PS_VALIDATE_METADATA_FROM_FLASH        1)
set(PS_MAX_ASSET_SIZE                      2048)
set(PS_NUM_ASSETS                          10)
set(PS_STACK_SIZE                          0x700)
set(CONFIG_TFM_CONN_HANDLE_MAX_NUM         8)
set(CONFIG_TFM_DOORBELL_API                0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tfm_config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/tfm_config.h)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DPROJECT_CONFIG_HEADER_FILE=${CMAKE_CURRENT_BINARY_DIR}/tfm_config.h
)
