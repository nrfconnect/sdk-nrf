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
set_property(GLOBAL PROPERTY
  tfm_PM_HEX_FILE $<TARGET_PROPERTY:tfm,TFM_S_HEX_FILE>
  )
