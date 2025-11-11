#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources(
  src/dl_parse.c
  src/dl_socket.c
  src/downloader.c
  src/dl_sanity.c
)

zephyr_library_sources_ifdef(
  CONFIG_DOWNLOADER_TRANSPORT_HTTP
  src/transports/http.c
)

zephyr_library_sources_ifdef(
  CONFIG_DOWNLOADER_TRANSPORT_COAP
  src/transports/coap.c
)

zephyr_library_sources_ifdef(
  CONFIG_DOWNLOADER_SHELL
  src/dl_shell.c
)

zephyr_include_directories(./include)
zephyr_linker_sources(RODATA dl_transports.ld)
