.. _nrf_rpc_utils:
.. _nrf_rpc_dev_info:

nRF RPC utility commands
########################

.. contents::
   :local:
   :depth: 2

This library implements various RPC commands that cannot be categorized into existing command libraries due to their distinct functionalities.

Configuration
*************

To use this library, enable the :kconfig:option:`CONFIG_NRF_RPC_UTILS` Kconfig option.

Additionally, you must specify the RPC role for this library by enabling one of the following Kconfig options:

* :kconfig:option:`CONFIG_NRF_RPC_UTILS_CLIENT` - Use this option for the client that sends RPC commands to a remote device.
* :kconfig:option:`CONFIG_NRF_RPC_UTILS_SERVER` - Use this option for the server that receives RPC commands.

Depending on your application's requirements, you can also include support for specific RPC commands with the following Kconfig options:

* :kconfig:option:`CONFIG_NRF_RPC_UTILS_CRASH_GEN`
* :kconfig:option:`CONFIG_NRF_RPC_UTILS_DEV_INFO`
* :kconfig:option:`CONFIG_NRF_RPC_UTILS_REMOTE_SHELL`

Dependencies
************

The library has the following dependency:

  * :ref:`nrf_rpc`

API documentation
*****************

| Header files: :file:`include/nrf_rpc/rpc_utils`
| Source files: :file:`subsys/nrf_rpc/rpc_utils`

.. doxygengroup:: nrf_rpc_utils
