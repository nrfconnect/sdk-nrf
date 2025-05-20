#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_TAG tag.c)
zephyr_library_sources_ifdef(CONFIG_NFC_TNEP_POLLER poller.c)

add_subdirectory_ifdef(CONFIG_NFC_TNEP_CH ch)

zephyr_linker_sources_ifdef(CONFIG_NFC_TNEP_TAG
  SECTIONS
  tag.ld
)
