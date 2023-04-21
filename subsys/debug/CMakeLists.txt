#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_CPU_LOAD		cpu_load)
add_subdirectory_ifdef(CONFIG_ETB_TRACE		etb_trace)
add_subdirectory_ifdef(CONFIG_PPI_TRACE		ppi_trace)
