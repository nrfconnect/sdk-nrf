#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(gzll_glue.c)
zephyr_library_sources_ifdef(CONFIG_GAZELL_PAIRING        gzp.c)
zephyr_library_sources_ifdef(CONFIG_GAZELL_PAIRING_DEVICE gzp_device.c)
zephyr_library_sources_ifdef(CONFIG_GAZELL_PAIRING_HOST   gzp_host.c)
