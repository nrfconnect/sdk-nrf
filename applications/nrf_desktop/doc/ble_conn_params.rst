.. _nrf_desktop_ble_conn_params:

Bluetooth LE connection parameters module
#########################################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth® LE connection parameters module for the following purposes:

* Update the connection parameters after the peripheral discovery.
* React on connection parameter update requests from the connected peripherals.
* Increase Bluetooth connection interval for peripherals while USB is suspended to reduce power consumption.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_conn_params_start
    :end-before: table_ble_conn_params_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module requires the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
Make sure that both :ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` options are enabled.
The |ble_conn_params| is enabled by the :ref:`CONFIG_DESKTOP_BLE_CONN_PARAMS_ENABLE <config_desktop_app_options>` option.
The option is implied by :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` together with other features used by a HID dongle that forwards the HID reports received over Bluetooth LE.

Enable :ref:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI <config_desktop_app_options>` to manage Bluetooth connections' parameters reacting on the USB state change.
The connection intervals for all of the Bluetooth connected peripherals are set to :ref:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI_VALUE <config_desktop_app_options>` (100 ms by default) while USB is suspended.
The connections' peripheral latencies are set to 0.
The connection parameter change is reverted when USB is active.
The :ref:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI <config_desktop_app_options>` is enabled by default.

Implementation details
**********************

The |ble_conn_params| receives the peripheral's connection parameters update request as :c:struct:`ble_peer_conn_params_event`.
The module updates only the connection latency.
The connection interval and supervision timeout are not changed according to the peripheral's request.

.. note::
   On the peripheral side, the Bluetooth connection latency is controlled by :ref:`nrf_desktop_ble_latency`.

LLPM connections
================

The Low Latency Packet Mode (LLPM) connection parameters are not supported by the standard Bluetooth.

The LLPM connection parameters update requires using vendor-specific HCI commands.
Moreover, the peripheral cannot request the LLPM connection parameters using Zephyr Bluetooth® API.

Connection interval update
==========================

After the :ref:`nrf_desktop_ble_discovery` completes the peripheral discovery, the |ble_conn_params| updates the connection parameters in the following manner:

* If the central and the connected peripheral both support the Low Latency Packet Mode (LLPM), the connection interval is set to **1 ms**.
* If neither the central nor the connected peripheral support LLPM, or if only one of them supports it, the interval is set to the following values:

  * **7.5 ms** if LLPM is not supported by the central or :kconfig:option:`CONFIG_BT_MAX_CONN` is set to value of 1.
    This is the shortest interval allowed by the standard Bluetooth.
  * **10 ms** otherwise.
    This is required to avoid Bluetooth Link Layer scheduling conflicts that could lead to HID report rate drop.
