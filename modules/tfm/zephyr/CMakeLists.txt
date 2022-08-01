#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (NOT CONFIG_TFM_BUILD_NS)
  zephyr_library_named(tfm_api_nrf)

  # The non-secure API files are located in a folder associated with the TF-M
  # build system. Usually the cmake INSTALL mechanism is used to move these
  # API source files to be built by the zephyr build system.
  # For convenience we instead refer directly to the source files here.
  set(src_dir ${CMAKE_CURRENT_LIST_DIR}/../tfm/boards/src)

  zephyr_library_sources_ifdef(CONFIG_TFM_PARTITION_PLATFORM
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
  if (CONFIG_TFM_REGRESSION_NS)
    set_property(GLOBAL PROPERTY
      tfm_nonsecure_PM_HEX_FILE $<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>
      )
  endif()
endif()

if (CONFIG_BOOTLOADER_MCUBOOT AND NOT CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY)
  # Configure the secondary partition to be non-secure
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_NS_SECONDARY=ON
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

if (CONFIG_TFM_SECURE_UART1)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DSECURE_UART1=ON
    )
else()
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DSECURE_UART1=OFF
    )
endif()

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
    -DPLATFORM_DEFAULT_CRYPTO_KEYS=OFF
    -DPLATFORM_DEFAULT_ROTPK=OFF
    -DPLATFORM_DEFAULT_IAK=OFF
    -DPLATFORM_DEFAULT_NV_SEED=OFF
    -DPLATFORM_DEFAULT_OTP=OFF
    -DPLATFORM_DEFAULT_OTP_WRITEABLE=OFF
    -DPLATFORM_DEFAULT_PROVISIONING=OFF
    -DTFM_DUMMY_PROVISIONING=OFF
    -DCRYPTO_NV_SEED=OFF
    -DPLATFORM_NV_COUNTER_MODULE_DISABLED=TRUE
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

# The ENGINE_BUF_SIZE holds the dynamic allocation buffer for
# TF-M, which is the equivalent of dynamic allocation in MBEDTLS
set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DCRYPTO_ENGINE_BUF_SIZE=${CONFIG_TFM_CRYPTO_ENGINE_BUF_SIZE}
    -DCRYPTO_CONC_OPER_NUM=${CONFIG_TFM_CRYPTO_CONC_OPER_NUM}
    -DCRYPTO_IOVEC_BUFFER_SIZE=${CONFIG_TFM_CRYPTO_IOVEC_BUFFER_SIZE}
)

zephyr_include_directories(${NRF_DIR}/include/tfm)
