#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
	src/download_client.c
	src/parse.c
	src/sanity.c
	src/client_socket.c
)

zephyr_library_sources_ifdef(
	CONFIG_DOWNLOAD_CLIENT_TRANSPORT_HTTP
	src/transports/http.c
)

zephyr_library_sources_ifdef(
	CONFIG_DOWNLOAD_CLIENT_TRANSPORT_COAP
	src/transports/coap.c
)

zephyr_library_sources_ifdef(
	CONFIG_DOWNLOAD_CLIENT_SHELL
	src/shell.c
)

zephyr_include_directories(./include)
zephyr_linker_sources(RODATA dlc_transports.ld)
