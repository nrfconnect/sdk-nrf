#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_FLPRCORE_REMOTE OR SB_CONFIG_PPRCORE_REMOTE)
  # Add a dependency so that the remote sample will be built and flashed first
  sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
  # Add dependency so that the remote image is flashed first.
  sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
endif()
