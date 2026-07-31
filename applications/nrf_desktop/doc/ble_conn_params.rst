.. _nrf_desktop_ble_conn_params:

Bluetooth LE connection parameters module
#########################################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth® LE connection parameters module for the following purposes:

* Update the connection parameters after the peripheral discovery.
* React on connection parameter update requests from the connected peripherals.
* Reduce power consumption while USB is suspended by increasing the Bluetooth connection interval for non-HID SCI connections, or by switching HID SCI connections to the LOW_POWER mode.

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

Enable :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI` to reduce dongle power consumption while USB is suspended by adjusting Bluetooth connection parameters based on the USB state.
The option is enabled by default.
See :ref:`nrf_desktop_ble_conn_params_usb_managed_ci` for details on how the module handles USB suspend and resume.

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

.. _nrf_desktop_ble_conn_params_usb_managed_ci:

USB managed connection parameters
=================================

When the :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI` Kconfig option is enabled, the |ble_conn_params| reacts to :c:struct:`usb_state_event` to reduce power consumption while USB is suspended.
The applied mechanism depends on whether a given connection uses HID SCI.

.. _nrf_desktop_ble_conn_params_usb_managed_ci_standard:

Standard connections
--------------------

For connections that do not use HID SCI, the module updates the Bluetooth connection parameters directly while USB is suspended:

* The connection interval is set to the value of :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI_VALUE` Kconfig option (100 ms by default).
* The peripheral latency is set to the value of :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_LATENCY_VALUE` Kconfig option (``1`` by default).
  The non-zero peripheral latency is used to prevent peripheral latency increase requests triggered by the :ref:`nrf_desktop_ble_latency` used on the peripheral's end.

When USB becomes active or disconnected, the module restores the connection parameters used during normal operation.

.. _nrf_desktop_ble_conn_params_usb_managed_ci_hid_sci:

HID SCI connections
-------------------

For connections that use HID SCI, the module does not update connection parameters directly.
Instead, it requests an appropriate HID SCI mode from the peripheral:

* On USB suspend, the module requests the LOW_POWER HID SCI mode.
* On USB resume or disconnect, the module requests the FAST HID SCI mode.

The :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI_VALUE` and :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_LATENCY_VALUE` Kconfig options do not apply to HID SCI connections.

If a peer switches out of the LOW_POWER mode while USB is suspended (for example, when the peer wakes up from powerdown), the module immediately requests the LOW_POWER mode again to avoid excessive power consumption.

.. _nrf_desktop_ble_conn_params_hid_sci:

HID SCI
=======

When the :option:`CONFIG_DESKTOP_HID_FORWARD_HID_SCI_ENABLE` Kconfig option is enabled, the |ble_conn_params| sets the promptless :option:`CONFIG_DESKTOP_BLE_CONN_PARAMS_HID_SCI_ENABLE` Kconfig option.
With this option set, the module sets default connection rate parameters on module initialization using :c:func:`bt_conn_le_conn_rate_set_defaults`.

For HID SCI connections, the module does not perform the standard connection parameter update.
Instead, it controls the connection parameters by requesting appropriate HID SCI modes from the peripheral, as required by the `HID Over GATT Profile Specification`_.
For details, see the :ref:`nrf_desktop_ble_conn_params_connection_interval_update` section.

For USB suspend and resume behavior, see :ref:`nrf_desktop_ble_conn_params_usb_managed_ci_hid_sci`.

LLPM connections
================

The Low Latency Packet Mode (LLPM) connection parameters are not supported by the standard Bluetooth.

The LLPM connection parameters update requires using vendor-specific HCI commands.
Moreover, the peripheral cannot request the LLPM connection parameters using Zephyr Bluetooth® API.

.. _nrf_desktop_ble_conn_params_connection_interval_update:

Connection interval update
==========================

After the :ref:`nrf_desktop_ble_discovery` completes the peripheral discovery, the |ble_conn_params| updates the connection parameters in the following manner:

* If the peripheral supports HID SCI, the parameter update is skipped and HID SCI FAST mode is requested as soon as the peripheral discovery completes.
  When :option:`CONFIG_DESKTOP_BLE_USB_MANAGED_CI` is enabled and USB is suspended, HID SCI LOW_POWER mode is requested instead.
* If the central and the connected peripheral both support the Low Latency Packet Mode (LLPM), the connection interval is set to **1 ms**.
* If neither the central nor the connected peripheral support LLPM, or if only one of them supports it, the interval is set to the following values:

  * **7.5 ms** if LLPM is not supported by the central or :kconfig:option:`CONFIG_BT_MAX_CONN` is set to value of 1.
    This is the shortest interval allowed by the standard Bluetooth.
  * **10 ms** otherwise.
    This is required to avoid Bluetooth Link Layer scheduling conflicts that could lead to HID report rate drop.
