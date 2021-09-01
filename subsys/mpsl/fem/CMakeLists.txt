#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MPSL_FEM_PIN_FORWARDER mpsl_fem_host.c)
zephyr_library_sources_ifdef(CONFIG_MPSL_FEM mpsl_fem.c)
