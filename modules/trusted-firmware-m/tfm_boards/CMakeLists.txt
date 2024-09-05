#
# Copyright (c) 2021 - 2024, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

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
    include
    ${partition_includes}
    ${board_includes}
    services/include
    ${ZEPHYR_NRF_MODULE_DIR}/include
)

target_sources(platform_s
  PRIVATE
    common/tfm_hal_platform.c
    $<$<OR:$<BOOL:${TFM_PARTITION_INITIAL_ATTESTATION}>,$<BOOL:${NRF_PROVISIONING}>>:${CMAKE_CURRENT_SOURCE_DIR}/common/attest_hal.c>
    common/assert.c
    $<$<NOT:$<BOOL:${PLATFORM_DEFAULT_OTP}>>:${CMAKE_CURRENT_SOURCE_DIR}/common/dummy_otp.c>
    $<$<NOT:$<BOOL:${PLATFORM_DEFAULT_SYSTEM_RESET_HALT}>>:${CMAKE_CURRENT_SOURCE_DIR}/common/tfm_hal_reset_halt.c>
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

target_sources(tfm_sprt
    PRIVATE
        $<$<NOT:$<BOOL:${TFM_SP_LOG_RAW_ENABLED}>>:${CMAKE_CURRENT_SOURCE_DIR}/common/dummy_tfm_sp_log_raw.c>
)

if (${TFM_PARTITION_CRYPTO})
  if(CRYPTO_TFM_BUILTIN_KEYS_DRIVER)
    target_sources(platform_crypto_keys
      PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/common/crypto_keys.c
    )
  endif()

  target_link_libraries(platform_crypto_keys
    PUBLIC
      platform_s
      # Link with tfm_sprt to get the include path for tfm_sp_log.h
      tfm_sprt
  )

   if (${CONFIG_HW_UNIQUE_KEY})
     target_compile_definitions(platform_s PUBLIC
       CONFIG_HW_UNIQUE_KEY
       $<$<BOOL:${CONFIG_HW_UNIQUE_KEY_RANDOM}>:CONFIG_HW_UNIQUE_KEY_RANDOM>)


       if((NRF_SOC_VARIANT STREQUAL nrf54l15) OR (target STREQUAL nrf54l15))
          target_sources(platform_crypto_keys
            PRIVATE
             ${ZEPHYR_NRF_MODULE_DIR}/lib/hw_unique_key/hw_unique_key_cracen.c
          )
       else()
          target_sources(platform_s
            PRIVATE
              ${ZEPHYR_NRF_MODULE_DIR}/lib/hw_unique_key/hw_unique_key.c
              ${ZEPHYR_NRF_MODULE_DIR}/lib/hw_unique_key/hw_unique_key_kmu.c
          )
       endif()
   endif()

  if (${TFM_PARTITION_INITIAL_ATTESTATION})
    target_sources(platform_s
      PRIVATE
        ${ZEPHYR_NRF_MODULE_DIR}/lib/identity_key/identity_key.c
    )
  endif()

  if(EXISTS platform_cc3xx)
	  target_link_libraries(platform_s PRIVATE platform_cc3xx)
  endif()

  # Needed in order to get crypto partition modules flags
  target_link_libraries(platform_s PRIVATE tfm_psa_rot_partition_crypto)
endif()

if (NRF_ALLOW_NON_SECURE_RESET)
  target_compile_definitions(platform_s PUBLIC NRF_ALLOW_NON_SECURE_RESET)
endif()

if (NRF_ALLOW_NON_SECURE_FAULT_HANDLING)
  target_compile_definitions(platform_s PUBLIC NRF_ALLOW_NON_SECURE_FAULT_HANDLING)
  target_sources(platform_s
    PRIVATE
      common/ns_fault_service.c
  )
endif()

if (CONFIG_NFCT_PINS_AS_GPIOS)
  target_compile_definitions(platform_s PUBLIC CONFIG_NFCT_PINS_AS_GPIOS)
endif()


if (CONFIG_NRF_TRACE_PORT)
  target_compile_definitions(platform_s PUBLIC ENABLE_TRACE)
endif()

if (CONFIG_NRF_APPROTECT_LOCK)
  target_compile_definitions(platform_s PUBLIC NRF_APPROTECT)
endif()

if (CONFIG_NRF_SECURE_APPROTECT_LOCK)
  target_compile_definitions(platform_s PUBLIC NRF_SECURE_APPROTECT)
endif()

if (CRYPTO_STORAGE_DISABLED AND TFM_PARTITION_CRYPTO AND NOT TFM_PARTITION_INTERNAL_TRUSTED_STORAGE)
  # Added here to satisfy the following requirement from tfm_crypto.yaml:
  #
  # "dependencies": [
  #   "TFM_INTERNAL_TRUSTED_STORAGE_SERVICE"
  # ]
  target_compile_definitions(platform_s PUBLIC
    TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_SID=0x00000070)
endif()

if(BL2)
  message(FATAL_ERROR "BL2 is not supported")
endif()

if(TFM_PARTITION_PLATFORM)
  set(src_dir ${CMAKE_CURRENT_LIST_DIR}/src)

  target_include_directories(platform_s
    PUBLIC
      include
      ${ZEPHYR_NRF_MODULE_DIR}/include/tfm
      ${ZEPHYR_NRF_MODULE_DIR}/include
  )

  target_sources(platform_s
    PRIVATE
      ${src_dir}/tfm_platform_system.c
      ${src_dir}/tfm_ioctl_s_api.c
  )

endif()

if(LOG_MEMORY_PROTECTION)
  target_sources(platform_s
	PRIVATE
	src/log_memory_protection.c
	)
endif()

#======================= Non Secure image ===========================

install(FILES       ns/CMakeLists.txt
        DESTINATION ${INSTALL_PLATFORM_NS_DIR})

if(TFM_PARTITION_PLATFORM)
  file(COPY board/device_cfg.h
            board/RTE_Device.h
       DESTINATION ${INSTALL_INTERFACE_INC_DIR})
endif()

file(COPY ${CMAKE_CURRENT_LIST_DIR}/common
     DESTINATION ${INSTALL_PLATFORM_NS_DIR})


if((NRF_SOC_VARIANT STREQUAL nrf54l15) OR (target STREQUAL nrf54l15))
  file(COPY ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_security/src/drivers/cracen/cracenpsa/include/cracen_psa_key_ids.h
       DESTINATION ${INSTALL_INTERFACE_INC_DIR})
  file(COPY ${ZEPHYR_NRF_MODULE_DIR}/subsys/nrf_security/src/drivers/cracen/cracenpsa/include/cracen_psa_kmu.h
       DESTINATION ${INSTALL_INTERFACE_INC_DIR})
endif()
