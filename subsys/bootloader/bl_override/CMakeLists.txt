#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK)
  add_overlay_config(
    mcuboot
    ${CMAKE_CURRENT_SOURCE_DIR}/override_external_mcuboot.conf
    )
endif()
