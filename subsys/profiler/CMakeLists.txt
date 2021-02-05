#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_PROFILER_SYSVIEW profiler_sysview.c)
zephyr_sources_ifdef(CONFIG_PROFILER_NORDIC profiler_nordic.c)
zephyr_sources_ifdef(CONFIG_SHELL profiler_common_shell.c)
