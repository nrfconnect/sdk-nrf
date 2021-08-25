#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(multicell_location.c)
zephyr_library_sources_ifdef(CONFIG_MULTICELL_LOCATION_HTTP http_request.c)
add_subdirectory(services)
