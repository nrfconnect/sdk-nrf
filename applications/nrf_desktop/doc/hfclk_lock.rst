.. _nrf_desktop_hfclk_lock:

High frequency clock lock hotfix module
#######################################

.. contents::
   :local:
   :depth: 2

Use the high frequency clock lock hotfix module to keep the high frequency clock enabled.
This reduces the latency before the first packet in a row is transmitted over Bluetooth, but it also increases the power consumption.
If this module is disabled, a start-up delay of around 1.5 ms will be added to the overall latency of the first packet.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hfclk_lock_start
    :end-before: table_hfclk_lock_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Enable the module with the :option:`CONFIG_DESKTOP_HFCLK_LOCK_ENABLE` Kconfig option.

Implementation details
**********************

The high frequency clock is disabled on ``power_down_event`` and reenabled on ``wake_up_event``.
This is done to reduce the power consumption.
