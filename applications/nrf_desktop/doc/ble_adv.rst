.. _nrf_desktop_ble_adv:

Bluetooth LE advertising module
###############################

.. contents::
   :local:
   :depth: 2

Use the |ble_adv| to control the Bluetooth LE advertising for the nRF Desktop peripheral device.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_adv_start
    :end-before: table_ble_adv_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Complete the following steps to enable the |ble_adv|:

1. Configure Bluetooth, as described in :ref:`nrf_desktop_bluetooth_guide`.
#. Define the Bluetooth device name (:option:`CONFIG_BT_DEVICE_NAME`).
#. Define the Bluetooth device appearance (:option:`CONFIG_BT_DEVICE_APPEARANCE`).
#. Set the :option:`CONFIG_DESKTOP_BLE_ADVERTISING_ENABLE` Kconfig option.

Using directed advertising
==========================

By default, the module uses indirect advertising.
Set the :option:`CONFIG_DESKTOP_BLE_DIRECT_ADV` option to use directed advertising.
The directed advertising can be used to call the selected peer device to connect as quickly as possible.

.. note::
   The nRF Desktop peripheral will not advertise directly towards a central that uses Resolvable Private Address (RPA).
   The peripheral does not read the Central Address Resolution GATT characteristic of the central, so the peripheral does not know if the remote device supports the address resolution of directed advertisements.

Changing advertising interval
=============================

Set the :option:`CONFIG_DESKTOP_BLE_FAST_ADV` Kconfig option to make the peripheral initially advertise with shorter interval in order to speed up finding the peripheral by Bluetooth Centrals.

* If the device uses indirect advertising, it will switch to slower advertising after the period of time defined in :option:`CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT` (in seconds).
* If the device uses directed advertising, the ``ble_adv`` module will receive :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enumerator:`PEER_STATE_CONN_FAILED` if the central does not connect during the predefined period of fast directed advertising.
  After the event is received, the device will switch to the low duty cycle directed avertising.

Switching to slower advertising is done to reduce the energy consumption.

Using Swift Pair
================

You can use the :option:`CONFIG_DESKTOP_BLE_SWIFT_PAIR` option to enable the Swift Pair feature.
The feature simplifies pairing the Bluetooth Peripheral with Windows 10 hosts.

Power down
==========

When the system goes to the Power Down state, the advertising stops.

If the Swift Pair feature is enabled, the device advertises without the Swift Pair data for additional :option:`CONFIG_DESKTOP_BLE_SWIFT_PAIR_GRACE_PERIOD` seconds to ensure that the user does not try to connect to the device that is no longer available.

Implementation details
**********************

The |ble_adv| uses Zephyr's :ref:`zephyr:settings_api` to store the information if the peer for the given local identity uses the Resolvable Private Address (RPA).

Reaction on Bluetooth peer operation
====================================

When the :ref:`nrf_desktop_ble_bond` triggers erase advertising or Bluetooth peer change, the |ble_adv| reacts on the received ``ble_peer_operation_event``.

* If there is a peer connected over Bluetooth, the |ble_adv| triggers disconnection and submits a :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enum:`PEER_STATE_DISCONNECTING` to let other application modules prepare for the planned disconnection.
* Otherwise the Bluetooth advertising with the newly selected Bluetooth local identity is started.

Avoiding connection requests from unbonded centrals when bonded
===============================================================

If the Bluetooth local identity currently in use already has a bond and the device uses indirect advertising, the advertising device will not set the General Discoverable flag.
If :option:`CONFIG_BT_WHITELIST` is enabled, the device will also whitelist incoming scan response data requests and connection requests.
This is done to prevent Bluetooth Centrals other than the bonded one from connecting with the device.
The nRF Desktop dongle scans for peripheral devices using the Bluetooth device name, which is provided in the scan response data.

.. |ble_adv| replace:: Bluetooth LE advertising module
