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
Make sure that both :option:`CONFIG_DESKTOP_ROLE_HID_DONGLE` and :option:`CONFIG_DESKTOP_BT_CENTRAL` options are enabled.
The |ble_conn_params| is enabled by the :option:`CONFIG_DESKTOP_BLE_CONN_PARAMS_ENABLE` option.
The option is implied by :option:`CONFIG_DESKTOP_BT_CENTRAL` together with other features used by a HID dongle that forwards the HID reports received over Bluetooth LE.

Enable :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI` to manage Bluetooth connections' parameters reacting on the USB state change.
The connection intervals for all of the Bluetooth connected peripherals are set to the value of :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI_VALUE` Kconfig option (100 ms by default) while USB is suspended.
The connections' peripheral latencies are set to the value of :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_LATENCY_VALUE` Kconfig option (``1`` by default).
The non-zero peripheral latency is used to prevent peripheral latency increase requests triggered by the :ref:`nrf_desktop_ble_latency` used on the peripheral's end.
The connection parameter change is reverted when USB is active or disconnected.
The :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI` is enabled by default.

Implementation details
**********************

After setting Bluetooth LE connection parameters using Bluetooth stack APIs, the module waits until the update is completed (for :c:struct:`ble_peer_conn_params_event` with :c:member:`ble_peer_conn_params_event.updated` set to ``true``) before performing subsequent connection parameter updates for a given connection.
Subsequent connection parameter update for a given connection can be done right after the previous one is completed.

Handling peripheral's requests
==============================

The |ble_conn_params| receives the peripheral's connection parameters update request as :c:struct:`ble_peer_conn_params_event` with :c:member:`ble_peer_conn_params_event.updated` set to ``false``.
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
