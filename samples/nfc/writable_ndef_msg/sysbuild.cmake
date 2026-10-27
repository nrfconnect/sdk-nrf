#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_PPRCORE_REMOTE)
  add_overlay_dts(${DEFAULT_IMAGE}
    ${APP_DIR}/boards/nrf9251dk_nrf9251_cpuapp_ppr.overlay)
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote_ppr)
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote_ppr)
endif()
