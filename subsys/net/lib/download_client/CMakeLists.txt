#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(
	src/download_client.c
	src/parse.c
	src/http.c
	src/sanity.c
)

zephyr_library_sources_ifdef(
	CONFIG_COAP
	src/coap.c
)

zephyr_library_sources_ifdef(
	CONFIG_DOWNLOAD_CLIENT_SHELL
	src/shell.c
)
