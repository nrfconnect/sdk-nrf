#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH flash)
add_subdirectory_ifdef(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RTT rtt)
add_subdirectory_ifdef(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART uart)
add_subdirectory_ifdef(CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_RAM ram)
