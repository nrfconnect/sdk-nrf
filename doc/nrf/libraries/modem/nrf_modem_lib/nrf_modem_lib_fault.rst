.. _nrf_modem_lib_fault:

Modem fault handling
####################

.. contents::
   :local:
   :depth: 2

If a fault occurs in the modem, the application is notified through the fault handler function that is registered with the Modem library during initialization.
This lets the application read the fault reason (in some cases the modem's program counter) and take the appropriate action.

On initialization (using :c:func:`nrf_modem_lib_init`), the Modem library integration layer registers the :c:func:`nrf_modem_fault_handler` function through the Modem library initialization parameters.
The behavior of the :c:func:`nrf_modem_fault_handler` function is controlled with the three following Kconfig options:

* :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_DO_NOTHING` - This option lets the fault handler log the Modem fault and return (default).
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_RESET_MODEM` - This option lets the fault handler schedule a workqueue task to reinitialize the modem and Modem library.
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC` - This option lets the fault handler function :c:func:`nrf_modem_fault_handler` be defined by the application, outside of the Modem library integration layer.
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_LTE_NET_IF` - This is the only option available when the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF` Kconfig option is selected.
  With this option, modem faults are signaled to the application as fatal network events (``NET_EVENT_CONN_IF_FATAL_ERROR``), to which the application subscribes by calling the :c:func:`net_mgmt_init_event_callback` function.

Implementing a custom fault handler
***********************************

If you want to implement a custom fault handler, enable the :kconfig:option:`CONFIG_NRF_MODEM_LIB_ON_FAULT_APPLICATION_SPECIFIC` Kconfig option and provide an implementation of the :c:func:`nrf_modem_fault_handler` function, considering the following points:

* The fault handler is called in an interrupt context.
* Re-initialization of the Modem library must be done outside of the fault handler.
