.. _nrf_modem_lib_diagnostic:

Diagnostic functionality
########################

The Modem library integration layer in |NCS| provides some memory diagnostic functionality that is enabled by the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG` option.

The application can retrieve runtime statistics for the library and TX memory region heaps by enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG` option and calling the :c:func:`nrf_modem_lib_diag_stats_get` function.
The application can schedule a periodic report of the runtime statistics of the library and TX memory region heaps, by enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_DUMP` option.
The application can log the allocations on the Modem library heap and the TX memory region by enabling the :kconfig:option:`CONFIG_NRF_MODEM_LIB_MEM_DIAG_ALLOC` option.
