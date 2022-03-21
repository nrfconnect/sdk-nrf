.. _nrf_desktop_failsafe:

Failsafe module
###############

.. contents::
   :local:
   :depth: 2

Use the failsafe module to erase the :ref:`zephyr:settings_api` partition after a fatal error.
This can be done to prevent broken settings from rendering the device permanently unusable.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_failsafe_start
    :end-before: table_failsafe_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Use the :ref:`CONFIG_DESKTOP_FAILSAFE_ENABLE <config_desktop_app_options>` option to enable the module.

Additionally, make sure that the following options are set as follows:

* :kconfig:option:`CONFIG_WATCHDOG` - The watchdog must be enabled.
* :kconfig:option:`CONFIG_RESET_ON_FATAL_ERROR` - The reset on fatal error must be disabled.

This is to ensure that the device will be blocked after a fatal error and then the watchdog will trigger the reboot.

After the reboot caused either by the watchdog or by the CPU lockup, the failsafe module erases the settings partition and clears the non-volatile settings data.
