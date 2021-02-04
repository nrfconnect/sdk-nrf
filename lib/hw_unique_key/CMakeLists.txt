#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(hw_unique_key.c)
ncs_add_partition_manager_config(pm.yml.huk_nrf52840)
