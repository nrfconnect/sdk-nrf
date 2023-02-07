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

* :kconfig:option:`CONFIG_CAF_BLE_ADV`
* :kconfig:option:`CONFIG_CAF_BLE_ADV_PM_EVENTS`
* :kconfig:option:`CONFIG_CAF_BLE_ADV_DIRECT_ADV`
* :kconfig:option:`CONFIG_CAF_BLE_ADV_FAST_ADV`
* :kconfig:option:`CONFIG_CAF_BLE_ADV_FAST_ADV_TIMEOUT`
* :kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST`
* :kconfig:option:`CONFIG_CAF_BLE_ADV_GRACE_PERIOD`

Read about some of these options in the following sections.

Enabling the module
===================

To enable the |ble_adv|, complete the following steps:

1. Enable and configure the :ref:`CAF Bluetooth LE state module <caf_ble_state>`.
#. Set the :kconfig:option:`CONFIG_CAF_BLE_ADV` Kconfig option.
#. Configure the advertising data and scan response data for undirected advertising.
   Both advertising data and scan response data are managed by Bluetooth LE advertising providers.
   See :ref:`bt_le_adv_prov_readme` for details.
   Also see :ref:`nrf_desktop_ble_adv` for an example of the Bluetooth LE advertising providers configuration defined by an application.

Using directed advertising
==========================

By default, the module uses indirect advertising.
Set the :kconfig:option:`CONFIG_CAF_BLE_ADV_DIRECT_ADV` option to use directed advertising.
The directed advertising can be used to call the selected peer device to connect as quickly as possible.

.. note::
   The module will not advertise directly towards a Central that uses Resolvable Private Address (RPA).
   The Bluetooth LE Peripheral does not read the Central Address Resolution GATT characteristic of the Bluetooth LE Central, so the Peripheral does not know if the remote device supports the address resolution of directed advertisements.

Changing advertising interval
=============================

Set the :kconfig:option:`CONFIG_CAF_BLE_ADV_FAST_ADV` Kconfig option to make the Peripheral initially advertise with a shorter interval.
This lets you speed up finding the Peripheral by Bluetooth Centrals.

* If the device uses indirect advertising, it will switch to slower advertising after the period of time defined in :kconfig:option:`CONFIG_CAF_BLE_ADV_FAST_ADV_TIMEOUT` (in seconds).
* If the device uses directed advertising, the |ble_adv| will receive :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enumerator:`PEER_STATE_CONN_FAILED` if the Central does not connect during the predefined period of fast directed advertising.
  The :c:struct:`ble_peer_event` is submitted by :ref:`caf_ble_state`.
  After the event is received, the device will switch to the low duty cycle directed advertising.

Switching to slower advertising is done to reduce the energy consumption.

Power-down
==========

When the system goes to the power-down state, the advertising either instantly stops or enters the grace period state.

.. _caf_ble_adv_grace_period:

Grace period
------------

The grace period is an advertising state, during which the advertising is still active, but the advertising data and scan response data can be modified to inform that system is about to go to the power-down state.

If any advertising data provider requests non-zero grace period time, the stopping of advertising on power-down is delayed by the requested time.
Instead of instantly stopping, the advertising enters the grace period.
After the grace period ends, the advertising stops.

The grace period is requested for example by the `Swift Pair`_ advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_SWIFT_PAIR`).
During the grace period, Swift Pair data is removed from the advertising packet and the device enters Swift Pair's cool-down phase.
This is done to ensure that the user does not try to connect to the device that is no longer available.

.. note::
   Make sure that :kconfig:option:`CONFIG_CAF_BLE_ADV_GRACE_PERIOD` Kconfig option is enabled if both following conditions are met:

   * Any of the providers requests the grace period.
   * :kconfig:option:`CONFIG_CAF_BLE_ADV_PM_EVENTS` is enabled.

   The :kconfig:option:`CONFIG_CAF_BLE_ADV_GRACE_PERIOD` is enabled by default if the Swift Pair advertising data provider is enabled in the configuration.

Implementation details
**********************

The |ble_adv| is used only by Bluetooth Peripheral devices.

The |ble_adv| uses Zephyr's :ref:`zephyr:settings_api` to store the information if the peer for the given local identity uses the Resolvable Private Address (RPA).

Undirected advertising data update
==================================

The module does not instantly update advertising and scan response payloads when either advertising data or scan response data (provided by :ref:`bt_le_adv_prov_readme`) is modified.
The module automatically gets new advertising data and scan response data from Bluetooth LE's advertising data provider subsystem only in the following cases:

* Bluetooth LE undirected advertising is started or restarted.
* Undirected advertising enters the :ref:`caf_ble_adv_grace_period`.

The payload update can be triggered by the application using :c:struct:`ble_adv_data_update_event`.
Make sure to submit the event after changing the Bluetooth data provided by a provider.

Reaction on Bluetooth peer operation
====================================

If the application supports Bluetooth LE bond management (:kconfig:option:`CONFIG_CAF_BLE_BOND_SUPPORTED`), the Bluetooth LE bond module defined for the application is used to control the Bluetooth bonds.
The Bluetooth LE bond module broadcasts information related to bond control using :c:struct:`ble_peer_operation_event`.

The |ble_adv| reacts on :c:struct:`ble_peer_operation_event` related to the Bluetooth peer change or erase advertising.
The module performs one of the following operations:

* If there is a peer connected over Bluetooth, the |ble_adv| triggers disconnection and submits a :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enum:`PEER_STATE_DISCONNECTING` to let other application modules prepare for the planned disconnection.
* Otherwise, the Bluetooth advertising with the newly selected Bluetooth local identity is started.

Avoiding connection requests from unbonded centrals when bonded
===============================================================

If :kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST` is enabled and the Bluetooth local identity currently in use already has a bond, the device will filter incoming scan response data requests and connection requests.
In that case, only the bonded peer can connect or request scan response data.
This is done to prevent Bluetooth Centrals other than the bonded one from connecting with the device.
