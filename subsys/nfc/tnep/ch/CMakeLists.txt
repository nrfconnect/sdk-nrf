#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_TAG tag.c)
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_POLLER poller.c)
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_CH common.c)
