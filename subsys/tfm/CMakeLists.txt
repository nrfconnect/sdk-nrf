#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set_property(GLOBAL PROPERTY
  bl2_PM_HEX_FILE ${CMAKE_BINARY_DIR}/tfm/bin/bl2.hex
  )

if (CONFIG_TFM_NATIVE_NS)
  set_property(GLOBAL PROPERTY
    signed_tfm_app_PM_HEX_FILE
    ${CMAKE_BINARY_DIR}/tfm/bin/tfm_s_ns_signed.hex
    )

  set_property(GLOBAL PROPERTY
    signed_tfm_app_PM_TARGET
    tfm_native_ns
    )
endif()
