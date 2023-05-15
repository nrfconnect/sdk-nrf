#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(at_cmd_custom.c)

zephyr_linker_sources(RWDATA at_cmd_custom.ld)
