#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_TFM_BL2)
  # Add the mcuboot hex file built by TFM as the contents of the bl2 partitions,
  # so it can be flashed together with the app/tfm hex files.
  set_property(GLOBAL PROPERTY
    bl2_PM_HEX_FILE ${CMAKE_BINARY_DIR}/tfm/bin/bl2.hex
    )
  if (CONFIG_TFM_REGRESSION_NS)
    set_property(GLOBAL PROPERTY
      app_primary_PM_HEX_FILE ${CMAKE_BINARY_DIR}/tfm_ns_signed.hex
      )
  else()
    set_property(GLOBAL PROPERTY
      app_primary_PM_HEX_FILE ${CMAKE_BINARY_DIR}/zephyr_ns_signed.hex
      )
  endif()
  set_property(GLOBAL PROPERTY
    tfm_primary_PM_HEX_FILE ${CMAKE_BINARY_DIR}/tfm_s_signed.hex
    )
else()
  if (CONFIG_TFM_REGRESSION_NS)
    set_property(GLOBAL PROPERTY
      app_primary_PM_HEX_FILE $<TARGET_PROPERTY:tfm,TFM_NS_HEX_FILE>
      )
  endif()
endif()

if (CONFIG_SETTINGS_FCB OR CONFIG_SETTINGS_NVS)
  # Configure the storage partition to be non-secure
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS -DNRF_NS_STORAGE=ON
  )
endif()

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


if (CONFIG_TFM_MINIMAL)
  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DPLATFORM_DUMMY_NV_COUNTERS=FALSE
    -DPS_TEST_NV_COUNTERS=OFF
    -DPLATFORM_NV_COUNTER_MODULE_DISABLED=TRUE
    )
endif()

zephyr_include_directories(${NRF_DIR}/include/tfm)
