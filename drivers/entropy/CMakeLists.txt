#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_ENTROPY_CC3XX entropy_cc3xx.c)
