#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#


add_subdirectory_ifdef(CONFIG_PPI_TRACE			ppi_trace)
add_subdirectory_ifdef(CONFIG_LOG_FRONTEND_GPIO		gpio_frontend)
