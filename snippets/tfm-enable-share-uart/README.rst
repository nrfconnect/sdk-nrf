.. _tfm_enable_share_uart:
.. _snippet-tfm-enable-share-uart:

Shared UART for application and TF-M logging snippet (tfm-enable-share-uart)
############################################################################

.. contents::
   :local:
   :depth: 2

The ``tfm-enable-share-uart`` snippet directs the TF-M logs to the UART (**UART0**) used by the application.

Overview
********

If you want to activate TF-M logging while having modem traces with the UART backend enabled, it can be useful to direct the TF-M logs to the UART (**UART0**) used by the application.
The snippet enables TF-M logging at the debug level (:kconfig:option:`CONFIG_TFM_SPM_LOG_LEVEL_DEBUG`), configures TF-M to share the secure UART0 instance with the application (:kconfig:option:`CONFIG_TFM_SECURE_UART0` and :kconfig:option:`CONFIG_TFM_SECURE_UART_SHARE_INSTANCE`), and enables the TF-M exception information dump (:kconfig:option:`CONFIG_TFM_EXCEPTION_INFO_DUMP`).

Usage
*****

To enable modem traces and TF-M logs, add the ``nrf91-modem-trace-uart`` and ``tfm-enable-share-uart`` snippets to the :term:`build configuration`.
You can do this in one of the following ways:

.. tabs::

   .. group-tab:: west

      To activate both modem traces and TF-M logs when building an application with west, use the following command pattern, where *board_target* corresponds to your board target:

      .. parsed-literal::
         :class: highlight

         west build --board *board_target* -S nrf91-modem-trace-uart -S tfm-enable-share-uart

   .. group-tab:: CMake

      To activate TF-M logs when building an application with CMake, add the following command to the CMake arguments:

      .. code-block:: console

         -D<image_name>_SNIPPET="tfm-enable-share-uart" [...]

      To build with |nRFVSC|, specify ``-D<image_name>_SNIPPET="tfm-enable-share-uart" [...]`` in the **Extra CMake arguments** field.

      See :ref:`cmake_options` for more details.
