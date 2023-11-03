.. _nrf_modem_lib_readme:

Modem library integration layer
###############################

The Modem library integration layer handles the integration of the Modem library into |NCS|.
The integration layer is constituted by the library wrapper and functionalities like socket offloading, OS abstraction, memory reservation by the Partition manager, handling modem traces, and diagnostics.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   nrf_modem_lib_wrapper.rst
   nrf_modem_lib_socket_offloading.rst
   nrf_modem_lib_os_abstraction.rst
   nrf_modem_lib_pm_integration.rst
   nrf_modem_lib_trace.rst
   nrf_modem_lib_fault.rst
   nrf_modem_lib_diagnostic
   nrf_modem_lib_api.rst
