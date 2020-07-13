#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()

zephyr_library_sources(
  mpsl_init.c
  )

zephyr_library_sources_ifdef(CONFIG_MPSL_FEM mpsl_fem.c)
