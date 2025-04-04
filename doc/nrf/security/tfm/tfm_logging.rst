.. _ug_tfm_logging:
.. _ug_tfm_manual_VCOM_connection:

TF-M logging
############

.. contents::
   :local:
   :depth: 2

TF-M employs two UART interfaces for logging: one for the :ref:`Secure Processing Environment<app_boards_spe_nspe>` (including MCUboot and TF-M), and one for the :ref:`Non-Secure Processing Environment<app_boards_spe_nspe>` (including user application).
By default, the logs arrive on different COM ports on the host PC.

Alternatively, you can configure the TF-M to connect to the same UART as the application with the :kconfig:option:`CONFIG_TFM_SECURE_UART0` Kconfig option.
Setting this Kconfig option makes TF-M logs visible on the application's VCOM, without manual connection.

The UART instance used by the application is ``0`` by default, and the TF-M UART instance is ``1``.
To change the TF-M UART instance to the same as that of the application's, use the :kconfig:option:`CONFIG_TFM_SECURE_UART0` Kconfig option.

.. note::

   When the TF-M and the user application use the same UART, the TF-M disables logging after it has booted and re-enables it again only to log a fatal error.

For nRF5340 DK devices, see :ref:`nrf5430_tfm_log`.
