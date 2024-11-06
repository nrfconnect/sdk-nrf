.. _nrf_desktop_hfclk_lock:

High frequency clock lock hotfix module
#######################################

.. contents::
   :local:
   :depth: 2

Use the high frequency clock lock hotfix module to keep the high frequency clock enabled.
This reduces the latency before the first packet in a row is transmitted over Bluetooth®, but it also increases the power consumption.
If this module is disabled, a startup delay of around 1.4 ms (0.85 ms in case of nRF54L Series SoCs) will be added to the overall latency of the first packet.

.. note::
  The module is deprecated.
  Use the :ref:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK <config_desktop_app_options>`) Kconfig option instead.
  Setting the peripheral latency Bluetooth LE connection parameter to ``0`` for a connection that uses Low Latency Packet Mode connection interval on peripheral leads to keeping the high frequency clock enabled.
  That mitigates the extra HID report latency caused by the high frequency clock startup delay.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_hfclk_lock_start
    :end-before: table_hfclk_lock_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module is not supported for nRF54H SoC Series (:kconfig:option:`CONFIG_SOC_SERIES_NRF54HX`).

Make sure that Bluetooth LE Low Latency Packet Mode (LLPM) is enabled in configuration (:kconfig:option:`CONFIG_CAF_BLE_USE_LLPM`).
Using LLPM connection parameters reduces HID data latency far more significantly than enabling the module.

Enable the module with the :ref:`CONFIG_DESKTOP_HFCLK_LOCK_ENABLE <config_desktop_app_options>` Kconfig option.

Implementation details
**********************

The high frequency clock is disabled on :c:struct:`power_down_event` and reenabled on :c:struct:`wake_up_event`.
This is done to reduce the power consumption.
