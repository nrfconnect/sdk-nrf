#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_PROFILER_SYSVIEW profiler_sysview.c)
zephyr_sources_ifdef(CONFIG_PROFILER_NORDIC profiler_nordic.c)
zephyr_sources_ifdef(CONFIG_SHELL profiler_common_shell.c)
