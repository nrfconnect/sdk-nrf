.. _nrf_desktop_constlat:

Constant latency hotfix module
##############################

.. contents::
   :local:
   :depth: 2

Enable the constant latency hotfix module to use a device configuration with constant latency interrupts.
This reduces the interrupt propagation time, but increases the power consumption.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_constlat_start
    :end-before: table_constlat_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To enable the module, use the :option:`CONFIG_DESKTOP_CONSTLAT_ENABLE` Kconfig option.

You can set the :option:`CONFIG_DESKTOP_CONSTLAT_DISABLE_ON_STANDBY` option to disable the constant latency interrupts when the device switches to the low power mode (on ``power_down_event``).
The constant latency interrupts are re-enabled on a ``wake_up_event``.
