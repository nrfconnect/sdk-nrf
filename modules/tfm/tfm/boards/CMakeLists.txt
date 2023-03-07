#-------------------------------------------------------------------------------
# Copyright (c) 2021, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
#-------------------------------------------------------------------------------

# NOTE: All code in this folder and subfolders is run as part of the TF-M
#       build, not in a Zephyr context.

set(NRF_BOARD_SELECTED True)

set(partition_includes
  partition
  ${CMAKE_BINARY_DIR}/../zephyr/include/generated
  )

set(board_includes
  board
  ${CMAKE_BINARY_DIR}/../zephyr/misc/generated/syscalls_links/include
  ${ZEPHYR_BASE}/include
  )

target_include_directories(platform_region_defs INTERFACE ${partition_includes})

target_compile_definitions(platform_s
  PRIVATE
  $<$<BOOL:${PLATFORM_DEFAULT_CRYPTO_KEYS}>:PLATFORM_DEFAULT_CRYPTO_KEYS>
  FIRMWARE_INFO_MAGIC=${FIRMWARE_INFO_MAGIC}
  EXT_API_MAGIC=${EXT_API_MAGIC}
  )

target_include_directories(platform_s
  PUBLIC
  ${partition_includes}
  ${board_includes}
  services/include
  ${NRF_DIR}/include
  )

target_sources(platform_s
  PRIVATE
  common/tfm_hal_platform.c
  $<$<BOOL:${TFM_PARTITION_INITIAL_ATTESTATION}>:${CMAKE_CURRENT_SOURCE_DIR}/common/attest_hal.c>
  common/assert.c
  $<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${CMAKE_CURRENT_SOURCE_DIR}/common/dummy_otp.c>
  )

target_link_libraries(platform_s
  PRIVATE
  tfm_sprt
 )

if (NOT ${PLATFORM_DEFAULT_PROVISIONING})
  if(NRF_PROVISIONING)
    target_compile_definitions(platform_s PUBLIC NRF_PROVISIONING)
    set(provisioning_source nrf_provisioning.c)
  else()
    set(provisioning_source dummy_provisioning.c)
  endif()

  target_sources(platform_s
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common/${provisioning_source}
    )
endif()

# Disabling UART stdout not supported in NS Image, which is always built even when not needed.
target_sources(platform_ns
  PRIVATE
  $<$<NOT:$<BOOL:${PLATFORM_DEFAULT_UART_STDOUT}>>:${CMAKE_CURRENT_SOURCE_DIR}/common/dummy_uart_stdout.c>
  )

if (${TFM_PARTITION_CRYPTO} AND (NOT ${PLATFORM_DEFAULT_CRYPTO_KEYS}))
  if (${CRYPTO_KEY_DERIVATION_MODULE_DISABLED})
    target_sources(platform_crypto_keys
      PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/common/dummy_crypto_keys.c
    )
  else ()
    target_sources(platform_crypto_keys
      PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/common/crypto_keys.c
    )

    target_sources(platform_s
      PRIVATE
        ${NRF_DIR}/lib/hw_unique_key/hw_unique_key.c
        ${NRF_DIR}/lib/hw_unique_key/hw_unique_key_kmu.c
    )
  endif()

  target_link_libraries(platform_crypto_keys PUBLIC platform_s)
endif()

if (${TFM_PARTITION_CRYPTO})
  target_link_libraries(platform_s PRIVATE platform_cc3xx)

  # NCSDK-19561 cc3xx libraries requires heap.
  target_compile_definitions(platform_region_defs
    INTERFACE
        ENABLE_HEAP
  )

  # Needed in order to get crypto partition modules flags
  target_link_libraries(platform_s PRIVATE tfm_psa_rot_partition_crypto)
endif()

if (NRF_ALLOW_NON_SECURE_RESET)
  target_compile_definitions(platform_s PUBLIC NRF_ALLOW_NON_SECURE_RESET)
endif()

if (CONFIG_NFCT_PINS_AS_GPIOS)
  target_compile_definitions(platform_s PUBLIC CONFIG_NFCT_PINS_AS_GPIOS)
endif()

target_compile_definitions(platform_s PUBLIC CONFIG_HW_UNIQUE_KEY CONFIG_HW_UNIQUE_KEY_RANDOM)

if (CRYPTO_STORAGE_DISABLED AND TFM_PARTITION_CRYPTO AND NOT TFM_PARTITION_INTERNAL_TRUSTED_STORAGE)
  # Added here to satisfy the following requirement from tfm_crypto.yaml:
  #
  # "dependencies": [
  #   "TFM_INTERNAL_TRUSTED_STORAGE_SERVICE"
  # ]
  target_compile_definitions(platform_s PUBLIC
    TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_SID=0x00000070)
endif()

target_include_directories(platform_ns PUBLIC ${board_includes})

if(BL2)
  message(FATAL_ERROR "BL2 is not supported")
endif()

if(TFM_PARTITION_PLATFORM)
  set(src_dir ${CMAKE_CURRENT_LIST_DIR}/src)

  target_include_directories(platform_s
    PUBLIC
    include
    ${NRF_DIR}/include/tfm
    ${NRF_DIR}/include
    )

  target_sources(platform_s
    PRIVATE
    ${src_dir}/tfm_platform_system.c
    ${src_dir}/tfm_ioctl_s_api.c
    )

  target_include_directories(platform_ns
    PUBLIC
    include
    ${NRF_DIR}/include/tfm
    ${NRF_DIR}/include
    )

  target_compile_definitions(platform_ns
    PUBLIC
    FIRMWARE_INFO_MAGIC=${FIRMWARE_INFO_MAGIC}
    EXT_API_MAGIC=${EXT_API_MAGIC}
    )

  target_sources(platform_ns
    PRIVATE
    ${src_dir}/tfm_ioctl_ns_api.c
    )
endif()

if(LOG_MEMORY_PROTECTION)
  target_sources(platform_s
	PRIVATE
	src/log_memory_protection.c
	)
endif()
