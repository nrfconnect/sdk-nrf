.. _ug_tfm_logging:
.. _ug_tfm_manual_VCOM_connection:

TF-M logging
############

.. contents::
   :local:
   :depth: 2

TF-M employs two UART interfaces for logging: one for the :ref:`Secure Processing Environment<app_boards_spe_nspe>` (including MCUboot and TF-M), and one for the :ref:`Non-Secure Processing Environment<app_boards_spe_nspe>` (including user application).

By default, the logs from Nordic Semiconductor's Development Kits (DKs) arrive on different serial ports on the host PC.

.. note::
   |serial_port_number_list|

The UART instances can vary by device family:

* nRF5340 and nRF91 Series: The application typically uses the UART instance ``0`` (``uart0``), and TF-M uses the UART instance ``1`` (``uart1``) by default.
* nRF54L Series devices that support TF-M: The application typically uses the UART instance ``20`` (``uart20``), and TF-M uses the UART instance ``30`` (``uart30``) by default.

For more information about the logging in the |NCS|, see :ref:`ug_logging`.

Configuring logging to the same UART as the application
*******************************************************

.. note::
   This option is not available to nRF54L Series devices.

For nRF5340 and nRF91 Series devices, you can configure TF-M to connect to the same UART as the application.
Setting the appropriate Kconfig option makes TF-M logs automatically visible on the application's UART.

To configure TF-M to connect to the same UART as the application, you can use the ``CONFIG_TFM_SECURE_UART`` Kconfig options, for example :kconfig:option:`CONFIG_TFM_SECURE_UART0` for the example above.

When building TF-M with logging enabled, UART instance used by TF-M must be disabled in the non-secure application, otherwise the non-secure application will fail to run.
The recommended way to do this is by setting it in an :file:`.overlay` file.
For example, assuming TF-M uses the ``uart1`` instance and :kconfig:option:`CONFIG_TFM_SECURE_UART0` is set, you can disable TF-M's UART instance in the devicetree overlay file like this:

.. code-block:: dts

   &uart1 {
	        status = "disabled";
   };

See :ref:`zephyr:set-devicetree-overlays` in the Zephyr documentation for more information about configuring devicetree overlays and :ref:`cmake_options` in the |NCS| documentation for how to provide overlay files to the build system.

.. note::
   When TF-M and the user application use the same UART, TF-M disables logging after it has booted and re-enables it again only to log a fatal error.

Disabling logging
*****************

To disable logging, enable the :kconfig:option:`CONFIG_TFM_LOG_LEVEL_SILENCE` option.

Logging on nRF5340 DK devices
*****************************

For nRF5340 DK devices, see :ref:`nrf5430_tfm_log`.
