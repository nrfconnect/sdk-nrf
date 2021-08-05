.. _caf_ble_adv:

CAF: Bluetooth LE advertising module
####################################

.. contents::
   :local:
   :depth: 2

When enabled for an application, the |ble_adv| is responsible for controlling the Bluetooth LE advertising for Bluetooth LE Peripheral device.

This module can only work together with the :ref:`CAF Bluetooth LE state module <caf_ble_state>`.
The Bluetooth LE state module is a core Bluetooth module in the :ref:`lib_caf` (CAF).

Configuration
*************

The following Kconfig options are available for this module:

* :kconfig:`CONFIG_CAF_BLE_ADV`
* :kconfig:`CONFIG_CAF_BLE_ADV_DEF_PATH`
* :kconfig:`CONFIG_CAF_BLE_ADV_PM_EVENTS`
* :kconfig:`CONFIG_CAF_BLE_ADV_DIRECT_ADV`
* :kconfig:`CONFIG_CAF_BLE_ADV_FAST_ADV`
* :kconfig:`CONFIG_CAF_BLE_ADV_FAST_ADV_TIMEOUT`
* :kconfig:`CONFIG_CAF_BLE_ADV_SWIFT_PAIR`
* :kconfig:`CONFIG_CAF_BLE_ADV_SWIFT_PAIR_GRACE_PERIOD`

Read about some of these options in the following sections.

Enabling the module
===================

To enable the |ble_adv|, complete the following steps:

1. Enable and configure the :ref:`CAF Bluetooth LE state module <caf_ble_state>`.
#. Define the Bluetooth device name using the :kconfig:`CONFIG_BT_DEVICE_NAME` Kconfig option.
#. Define the Bluetooth device appearance using the :kconfig:`CONFIG_BT_DEVICE_APPEARANCE` Kconfig option.
#. Set the :kconfig:`CONFIG_CAF_BLE_ADV` Kconfig option.
#. Create a configuration file that defines Bluetooth LE advertising data.
#. In the configuration file, define the following arrays of :c:struct:`bt_data`:

   * ``ad_unbonded`` - Defines undirected advertising data when unbonded.
   * ``ad_bonded``- Defines undirected advertising data when bonded.
   * ``sd``- Defines scan response data.

   For example, the file contents should look like follows:

   .. code-block:: c

      #include <zephyr.h>
      #include <bluetooth/bluetooth.h>

      static const struct bt_data ad_unbonded[] = {
            BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
            BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                          0x0f, 0x18,          /* Battery Service */
            ),
      };

      static const struct bt_data ad_bonded[] = {
            BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
            BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                          0x0f, 0x18,	/* Battery Service */
            ),
      };

      static const struct bt_data sd[] = {};

#. Specify the path to the configuration file with the :kconfig:`CONFIG_CAF_BLE_ADV_DEF_PATH` Kconfig option.

.. note::
    The configuration file should be included only by the configured module.
    Do not include the configuration file in other source files.

Using directed advertising
==========================

By default, the module uses indirect advertising.
Set the :kconfig:`CONFIG_CAF_BLE_ADV_DIRECT_ADV` option to use directed advertising.
The directed advertising can be used to call the selected peer device to connect as quickly as possible.

.. note::
   The module will not advertise directly towards a Central that uses Resolvable Private Address (RPA).
   The Bluetooth LE Peripheral does not read the Central Address Resolution GATT characteristic of the Bluetooth LE Central, so the Peripheral does not know if the remote device supports the address resolution of directed advertisements.

Changing advertising interval
=============================

Set the :kconfig:`CONFIG_CAF_BLE_ADV_FAST_ADV` Kconfig option to make the Peripheral initially advertise with a shorter interval.
This lets you speed up finding the Peripheral by Bluetooth Centrals.

* If the device uses indirect advertising, it will switch to slower advertising after the period of time defined in :kconfig:`CONFIG_CAF_BLE_ADV_FAST_ADV_TIMEOUT` (in seconds).
* If the device uses directed advertising, the |ble_adv| will receive :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enumerator:`PEER_STATE_CONN_FAILED` if the Central does not connect during the predefined period of fast directed advertising.
  The :c:struct:`ble_peer_event` is submitted by :ref:`caf_ble_state`.
  After the event is received, the device will switch to the low duty cycle directed advertising.

Switching to slower advertising is done to reduce the energy consumption.

Using Swift Pair
================

You can use the :kconfig:`CONFIG_CAF_BLE_ADV_SWIFT_PAIR` option to enable the Swift Pair feature.
The feature simplifies pairing the Bluetooth Peripheral with Windows 10 hosts.

.. note::
   Make sure to add the Swift Pair data to advertising packets for unbonded device in the configuration file if you enable :kconfig:`CONFIG_CAF_BLE_ADV_SWIFT_PAIR` option.
   The Swift Pair data must be added as the last member of ``ad_unbonded`` array.

Power-down
==========

When the system goes to the Power-down state, the advertising stops.

If the Swift Pair feature is enabled with :kconfig:`CONFIG_CAF_BLE_ADV_SWIFT_PAIR`, the device advertises without the Swift Pair data for additional :kconfig:`CONFIG_CAF_BLE_ADV_SWIFT_PAIR_GRACE_PERIOD` seconds to ensure that the user does not try to connect to the device that is no longer available.

Implementation details
**********************

The |ble_adv| is used only by Bluetooth Peripheral devices.

The |ble_adv| uses Zephyr's :ref:`zephyr:settings_api` to store the information if the peer for the given local identity uses the Resolvable Private Address (RPA).

Reaction on Bluetooth peer operation
====================================

If the application supports Bluetooth LE bond management (:kconfig:`CONFIG_CAF_BLE_BOND_SUPPORTED`), the Bluetooth LE bond module defined for the application is used to control the Bluetooth bonds.
The Bluetooth LE bond module broadcasts information related to bond control using :c:struct:`ble_peer_operation_event`.

The |ble_adv| reacts on :c:struct:`ble_peer_operation_event` related to the Bluetooth peer change or erase advertising.
The module performs one of the following operations:

* If there is a peer connected over Bluetooth, the |ble_adv| triggers disconnection and submits a :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enum:`PEER_STATE_DISCONNECTING` to let other application modules prepare for the planned disconnection.
* Otherwise, the Bluetooth advertising with the newly selected Bluetooth local identity is started.

Avoiding connection requests from unbonded centrals when bonded
===============================================================

If :kconfig:`CONFIG_BT_WHITELIST` is enabled and Bluetooth local identity that is in use already has a bond, the device will whitelist incoming scan response data requests and connection requests.
This is done to prevent Bluetooth Centrals other than the bonded one from connecting with the device.

.. |ble_adv| replace:: Bluetooth LE advertising module
