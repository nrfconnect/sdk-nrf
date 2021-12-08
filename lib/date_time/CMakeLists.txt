#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(date_time.c)
zephyr_library_sources(date_time_core.c)
zephyr_library_sources_ifdef(CONFIG_DATE_TIME_NTP date_time_ntp.c)
zephyr_library_sources_ifdef(CONFIG_DATE_TIME_MODEM date_time_modem.c)
