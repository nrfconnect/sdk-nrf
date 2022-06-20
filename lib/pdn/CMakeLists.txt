#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(pdn.c)
zephyr_library_sources_ifdef(CONFIG_PDN_ESM_STRERROR esm.c)
