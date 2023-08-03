#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
  src/mcumgr_smp_client.c
  )

zephyr_library_sources_ifdef(CONFIG_NRF_MCUMGR_SMP_CLIENT_SHELL
  src/mcumgr_smp_client_shell.c
  )

zephyr_include_directories(./include)
