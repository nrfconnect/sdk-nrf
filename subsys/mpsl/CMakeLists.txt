#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  mpsl_init.c
  )

zephyr_library_sources_ifdef(CONFIG_MPSL_CX mpsl_cx.c)
zephyr_library_sources_ifdef(CONFIG_MPSL_CX_THREAD cx/thread/mpsl_cx_thread.c)
zephyr_library_sources_ifdef(CONFIG_MPSL_FEM mpsl_fem.c)
