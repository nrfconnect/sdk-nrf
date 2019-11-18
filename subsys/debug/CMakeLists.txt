#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#


add_subdirectory_ifdef(CONFIG_PPI_TRACE            ppi_trace)
add_subdirectory_ifdef(CONFIG_STACK_SIZE_ANALYZER  stack_size_analyzer)
