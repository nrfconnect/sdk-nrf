.. _ug_nrf91_snippet:

Snippets for an nRF91 Series device
###################################

.. contents::
   :local:
   :depth: 2


:ref:`snippets` are tailored for tracing on the nRF91 Series devices but can work with other boards as well.
On nRF91 Series devices, snippets are used for the following functionalities:

* Modem tracing with the flash backend
* Modem tracing with the UART backend
* To activate TF-M logging while having modem traces enabled

.. _nrf91_modem_trace_ext_flash_snippet:

nRF91 modem traces with flash backend using snippets
====================================================

Snippet enables modem tracing, the flash backend, and external flash and configures them to store modem traces to a dedicated partition on the external flash for supported boards.
To change the partition size, the project needs to configure the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_FLASH_PARTITION_SIZE` Kconfig option.

The following build targets have support for this snippet:

* ``nrf9151dk/nrf9151/ns``
* ``nrf9161dk/nrf9161/ns``
* ``nrf9160dk/nrf9160/ns``
* ``nrf9131ek/nrf9131/ns``

To enable modem traces with the flash backend, use the following command:

.. parsed-literal::
   :class: highlight

   west build --board *build target* -S nrf91-modem-trace-ext-flash

.. _nrf91_modem_trace_uart_snippet:

nRF91 modem tracing with UART backend using snippets
====================================================

Snippet enables the :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE` Kconfig option and chooses the Zephyr UART driver for the backend, with the necessary Kconfig options.
The snippet also enables the UART1 peripheral with a baud rate of 1 Mbd and hardware flow control enabled.
If this configuration does not match your requirements, you can add a snippet or Kconfig and devicetree overlays to your application with the desired setup.
To enable modem tracing with the UART trace backend on a nRF91 device, add the ``nrf91-modem-trace-uart`` snippet to the :term:`build configuration`.
This can be done in one of the following ways:

With west
---------

To add the modem trace UART snippet when building an application with west, use the following command:

.. code-block:: console

   west build --board <your_board> -S nrf91-modem-trace-uart

With CMake
----------

To add the modem trace UART snippet when building an application with CMake, add the following command to the CMake arguments:

.. code-block:: console

   -DSNIPPET="nrf91-modem-trace-uart" [...]

To build with the |nRFVSC|, specify ``-DSNIPPET="nrf91-modem-trace-uart" [...]`` in the **Extra CMake arguments** field.

See :ref:`cmake_options` for more details.

.. _tfm_enable_share_uart:

Shared UART for application and TF-M logging
============================================

If you want to activate TF-M logging while having modem traces enabled, it can be useful to direct the TF-M logs to the UART (**UART0**) used by the application.
To activate both modem traces and TF-M logs, use the following command:

.. parsed-literal::
   :class: highlight

   west build --board *build target* -S nrf91-modem-trace-uart -S tfm-enable-share-uart
