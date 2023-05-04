#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_NRF_MODEM_LIB_TRACE_SHELL)
  target_include_directories(app PRIVATE .)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modem_trace_shell.c)
  target_sources_ifdef(CONFIG_MEMFAULT app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modem_trace_memfault.c)
endif()
