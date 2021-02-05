#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_AT_CMD)
    zephyr_library()
    zephyr_library_sources(at_cmd.c)
endif()
