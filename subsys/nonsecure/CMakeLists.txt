#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS secure_services_ns.c)
