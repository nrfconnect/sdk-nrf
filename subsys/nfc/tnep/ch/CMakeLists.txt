#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_TAG tag.c)
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_POLLER poller.c)
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_CH common.c)
