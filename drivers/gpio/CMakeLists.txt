#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_GPIO_NRFE gpio_nrfe.c)
