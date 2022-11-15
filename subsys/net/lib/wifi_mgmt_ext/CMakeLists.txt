#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(wifi_mgmt_ext.c)
zephyr_library_sources_ifdef(
	CONFIG_WIFI_CREDENTIALS_SHELL_AUTOCONNECT
	wifi_cred_shell_autoconnect.c)
