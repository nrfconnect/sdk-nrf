#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_NRF_MODEM_LIB_ZEPHYR_MODEM)
  # Standalone CMUX proxy — no nrfxlib modem, no partition manager sources
  zephyr_linker_sources(RODATA nrf_modem_lib.ld)
  zephyr_linker_sources_ifdef(CONFIG_NRF_MODEM_LIB_CFUN_HOOKS RODATA cfun_hooks.ld)
  add_subdirectory(zephyr_modem)
  return()
endif()

zephyr_library()
zephyr_library_sources(nrf_modem_lib.c)
zephyr_library_sources(nrf_modem_os.c)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_NRF92 nrf_modem_os_rpc.c)
zephyr_library_sources_ifdef(CONFIG_NRF_MODEM_LIB_CFUN_HOOKS cfun_hooks.c)
zephyr_library_sources_ifdef(CONFIG_NRF_MODEM_LIB_MEM_DIAG diag.c)
zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS nrf9x_sockets.c)
zephyr_library_include_directories_ifdef(CONFIG_NET_SOCKETS ${ZEPHYR_BASE}/subsys/net/lib/sockets)

add_subdirectory_ifdef(CONFIG_NRF_MODEM_LIB_NET_IF lte_net_if)
add_subdirectory_ifdef(CONFIG_SHELL shell)

if(CONFIG_NRF_MODEM_LIB_TRACE)
  zephyr_library_sources(nrf_modem_lib_trace.c)
  add_subdirectory(trace_backends)
endif()

zephyr_library_sources(fault.c)
zephyr_library_sources(sanity.c)

zephyr_linker_sources(RODATA nrf_modem_lib.ld)
zephyr_linker_sources_IFDEF(CONFIG_NRF_MODEM_LIB_CFUN_HOOKS RODATA cfun_hooks.ld)
