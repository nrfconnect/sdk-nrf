# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_named(otperf)

zephyr_library_sources(
    otperf_common.c
    otperf_udp_uploader.c
)

zephyr_library_sources_ifdef(CONFIG_OTPERF_SERVER
    otperf_udp_receiver.c
    otperf_session.c
)

zephyr_library_sources_ifdef(CONFIG_OTPERF_SHELL otperf_shell.c)
