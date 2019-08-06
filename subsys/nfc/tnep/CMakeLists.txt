#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP tag.c)
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP protocol_timer.c)
