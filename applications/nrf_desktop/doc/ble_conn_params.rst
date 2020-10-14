.. _nrf_desktop_ble_conn_params:

Bluetooth LE connection parameters module
#########################################

.. contents::
   :local:
   :depth: 2

Use the |ble_conn_params| to:

* Update the connection parameters after the peripheral discovery.
* React on connection parameter update requests from the connected peripherals.

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
The module is automatically enabled for every nRF Desktop central device (:option:`CONFIG_BT_CENTRAL`).

Implementation details
**********************

The |ble_conn_params| receives the peripheral's connection parameters update request as ``ble_peer_conn_params_event``.
The module updates only the connection latency.
The connection interval and supervision time-out are not changed according to the peripheral's request.

.. note::
   On the peripheral side, the Bluetooth connection latency is controlled by :ref:`nrf_desktop_ble_latency`.

LLPM connections
================

The Low Latency Packet Mode (LLPM) connection parameters are not supported by the standard Bluetooth.

The LLPM connection parameters update requires using Vendor Sprecific HCI commands.
Moreover, the peripheral cannot request the LLPM connection parameters using Zephyr Bluetooth API.

Connection interval update
==========================

After the :ref:`nrf_desktop_ble_discovery` completes the peripheral discovery, the |ble_conn_params| updates the connection parameters in the following manner:

* If the central and the connected peripheral both support the Low Latency Packet Mode (LLPM), the connection interval is set to **1 ms**.
* If neither the central nor the connected peripheral support LLPM, or if only one of them supports it, the interval is set to the following values:

  * **7.5 ms** if LLPM is not supported by the central or :option:`CONFIG_BT_MAX_CONN` is set to value 2 or lower.
    This is the shortest interval allowed by the standard Bluetooth.
  * **10 ms** otherwise.
    This is required to avoid Bluetooth Link Layer scheduling conflicts that could lead to HID report rate drop.

.. |ble_conn_params| replace:: Bluetooth LE connection parameters module
