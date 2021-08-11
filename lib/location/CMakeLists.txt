#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(location.c)
#TODO add if METHOD_GNSS is enabled
zephyr_library_sources(method_gnss.c)
#TODO add if METHOD_CELLULAR is enabled
zephyr_library_sources_ifdef(CONFIG_METHOD_CELLULAR method_cellular.c)
