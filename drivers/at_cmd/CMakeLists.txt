#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(CONFIG_AT_CMD)
    zephyr_library()
    zephyr_library_sources(at_cmd.c)
endif()
