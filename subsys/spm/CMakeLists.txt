#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources(spm.c)
zephyr_sources_ifdef(CONFIG_SPM_SECURE_SERVICES secure_services.c)
zephyr_linker_sources(SECTIONS secure_services.ld)
