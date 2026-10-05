#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(dect_icmp_ping_lib)
zephyr_library_sources(
  src/dect_icmp_ping.c
  src/dect_icmp_ping_shell.c
)
