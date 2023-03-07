.. _nrf_desktop_ble_bond:

Bluetooth LE bond module
########################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth® LE bond module to manage the Bluetooth peers and bonds.
The module controls the following operations:

* Switching between peers (only for nRF Desktop peripheral).
* Triggering scanning for new peers (only for nRF Desktop central).
* Erasing the Bluetooth bonds.

.. note::
   The nRF Desktop uses the application-specific |ble_bond| implementation.
   The application does not rely on the :ref:`caf_ble_bond` implementation.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_bond_start
    :end-before: table_ble_bond_end

.. note::
    |nrf_desktop_module_event_note|

Local identities
****************

The nRF Desktop device uses the following types of local identities:

* Application local identity - Used by the application modules.
  The currently selected application local identity and its state are presented to the user by the :ref:`nrf_desktop_led_state`.
* Bluetooth local identity - Used by Zephyr's :ref:`zephyr:bluetooth` API.
  Every Bluetooth local identity uses its own Bluetooth address and Identity Resolving Key (IRK).
  From the remote device's perspective, every Bluetooth local identity of a given peripheral is observed as a separate Bluetooth device.

Both application local identities and Bluetooth local identities are identified using IDs.
Both IDs related to the given Bluetooth peer operation are propagated in ``ble_peer_operation_event``.

The identity usage depends on the device type:

  * nRF Desktop central uses only one application local identity and only one Bluetooth local identity (the default ones).
  * nRF Desktop peripheral uses multiple local identities.

Every application local identity is associated with exactly one Bluetooth local identity.
The |ble_bond| stores the mapping from the application local identities to the Bluetooth local identities in the ``bt_stack_id_lut`` array.
The mapping changes only after a successful erase advertising.

Only one Bluetooth peer can be bonded with a given local identity.

Also, only one of the application local identities is selected at a time.
When the device changes the selected Bluetooth peer, it actually switches its own local identity.
The old peer is disconnected by the application, Bluetooth advertising is started, and the new peer connects.

Module states
*************

The |ble_bond| is implemented as a state machine.
Every transition is triggered by an :ref:`app_event_manager` event with a predefined value.
Some transitions can be also triggered by internal timeout.
For example, the transition from :c:enumerator:`STATE_ERASE_PEER` to :c:enumerator:`STATE_IDLE` can be triggered by ``click_event``, ``selector_event``, or an internal timeout.

When the transition occurs:

a. The :c:struct:`ble_peer_operation_event` with the defined :c:member:`ble_peer_operation_event.op` is submitted.
   For example, when the user confirms the erase advertising, the :c:struct:`ble_peer_operation_event` is submitted with :c:member:`ble_peer_operation_event.op` set to :c:enumerator:`PEER_OPERATION_ERASE_ADV`.
#. The currently selected application local identity is updated (if anything changed).

The following diagram shows states and transitions between these states after the module is initialized:

.. figure:: /images/nrf_desktop_ble_bond.svg
   :alt: nRF Desktop Bluetooth LE bond module state diagram

   nRF Desktop Bluetooth LE bond module state diagram

.. note::
  The diagram does not present the states related to module going into standby (:c:enum:`STATE_STANDBY`, :c:enum:`STATE_DISABLED_STANDBY`, :c:enum:`STATE_DONGLE_STANDBY`).
  For more information about the standby states, see `Standby states`_.

Receiving ``click_event`` with a click type that is not included in the schematic will result in cancelling the ongoing operation and returning to :c:enumerator:`STATE_IDLE` or :c:enumerator:`STATE_DONGLE`.
This does not apply to :c:enumerator:`STATE_DONGLE`.
In this state, all the peer operations triggered by ``click_event`` are disabled.

Idle (:c:enumerator:`STATE_IDLE`)
=================================

This is the default state of the module.

When in this state, the following transitions are possible:

* Start peer erase with :c:enumerator:`PEER_OPERATION_ERASE` by triggering a ``click_event`` with the ``CLICK_LONG`` click type.
* Start erase advertising with :c:enumerator:`PEER_OPERATION_ERASE_ADV` by triggering a ``click_event`` with the ``ON_START_CLICK(CLICK_LONG)`` click type.
* Select next peer with :c:enumerator:`PEER_OPERATION_SELECT` by triggering a ``click_event`` with the ``CLICK_SHORT`` click type.
* Select the dongle peer with :c:enumerator:`PEER_OPERATION_SELECTED` by triggering a ``selector_event`` with the selector in the :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_POS <config_desktop_app_options>` position.

It is also possible to trigger looking for a new peer with :c:enumerator:`PEER_OPERATION_SCAN_REQUEST` with a ``click_event`` of the ``CLICK_SHORT`` click type.

.. _ble_bond_peer_erasing:

Peer erasing (:c:enumerator:`STATE_ERASE_PEER`)
===============================================

Peer erasing is triggered with the reception of a ``click_event`` of the ``CLICK_LONG`` type.

Depending on the device type:

* The nRF Desktop central erases all bonded peers on the erase confirmation.
* The nRF Desktop peripheral starts erase advertising on the erase confirmation.

.. _ble_bond_erase_adv:

Erase advertising (:c:enumerator:`STATE_ERASE_ADV`)
===================================================

The erase advertising is used to make sure that the user is able to switch back to the old peer if the new one fails to connect or bond.
The peripheral uses an additional Bluetooth local identity that is not associated with any application local identity.
The peripheral resets the identity and starts advertising using it.

If a new peer successfully connects, establishes security, and bonds, the current application local identity switches to using the Bluetooth local identity that was used during the erase advertising.
The new peer is associated with the currently used application local identity.

After a timeout or on user request, the erase advertising is stopped.
The application local identity still uses the Bluetooth local identity that was associated with it before the erase advertising.

.. note::
   The erase advertising timeout can be extended in case a new peer connects.
   This ensures that a new peer will have time to establish the Bluetooth security level.

   The timeout is increased to a bigger value when the passkey authentication is enabled (:ref:`CONFIG_DESKTOP_BLE_ENABLE_PASSKEY <config_desktop_app_options>`).
   This gives the end user enough time to enter the passkey.

Peer selection (:c:enumerator:`STATE_SELECT_PEER`)
==================================================

In this state, the module waits for confirmation before the peer is switched.
The connected peer remains unchanged until the selection is confirmed.

Dongle states
=============

The module can go into one of the following dongle states:

* :c:enumerator:`STATE_DONGLE` - This is the default state of the module when the hardware selector used to select the dongle peer is placed in the :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_POS <config_desktop_app_options>` position.
  This state is used for connection with the nRF Desktop dongle.
* :c:enumerator:`STATE_DONGLE_STANDBY` - The Bluetooth LE bond module is suspended when the dongle peer is selected.
  See :ref:`ble_bond_standby_states`.
* :c:enumerator:`STATE_DONGLE_ERASE_PEER` - As :ref:`ble_bond_peer_erasing`, but for erasing dongle peer.
* :c:enumerator:`STATE_DONGLE_ERASE_ADV` - As :ref:`ble_bond_erase_adv`, but for erasing dongle peer.
  Similarly, the device waits with erasing the previous peer until it is connected and bonded with a new one.

.. figure:: /images/nrf_desktop_ble_bond_dongle.svg
   :alt: nRF Desktop Bluetooth LE bond module dongle state diagram

   nRF Desktop Bluetooth LE bond module dongle state diagram

.. _ble_bond_standby_states:

Standby states
==============

The module can go into one of the following standby states to make sure that the peer operations are not triggered when the device is suspended by :ref:`nrf_desktop_power_manager`:

* :c:enumerator:`STATE_DISABLED_STANDBY` - The module is suspended before initialization.
* :c:enumerator:`STATE_DONGLE_STANDBY` - The module is suspended when the dongle peer is selected.
* :c:enumerator:`STATE_STANDBY` - The module is suspended when other Bluetooth peers are selected.

Going into the standby states and leaving these states happens in reaction to the following events:

* ``power_down_event`` - On this event, the module goes into one of the standby states and the ongoing peer operation is cancelled.
* ``wake_up_event`` - On this event, the module returns from the standby state.

.. figure:: /images/nrf_desktop_ble_bond_standby.svg
   :alt: nRF Desktop Bluetooth LE bond module standby state diagram

   nRF Desktop Bluetooth LE bond module standby state diagram

Configuration
*************

The module requires the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
Set the :ref:`CONFIG_DESKTOP_BLE_BOND_ENABLE <config_desktop_app_options>` option to enable the module.

You can control the connected peers using the following methods:

* `Peer control using a button`_
* `Peer control using a hardware selector`_

You can use both methods, but the hardware selector has precedence.

When configuring the module, you can also enable `Default Bluetooth local identity on peripheral`_.

Peer control using a button
===========================

Complete the following steps to let the user control Bluetooth peers using the dedicated button:

1. Set the :ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL <config_desktop_app_options>` option to enable the feature.
#. Configure the :ref:`caf_buttons`.
#. Define the button's key ID as :ref:`CONFIG_DESKTOP_BLE_PEER_CONTROL_BUTTON <config_desktop_app_options>`.
#. Add the button to the :ref:`nrf_desktop_click_detector` configuration, because the |ble_bond| reacts on ``click_event``.

The following peer operations can be enabled:

* :ref:`CONFIG_DESKTOP_BLE_PEER_ERASE <config_desktop_app_options>` - Bluetooth LE peer erase triggered at any time.
* :ref:`CONFIG_DESKTOP_BLE_PEER_ERASE_ON_START <config_desktop_app_options>` - Erase advertising triggered by long press of the predefined button on system start.
  This option can be used only by nRF Desktop peripheral.
* :ref:`CONFIG_DESKTOP_BLE_PEER_SELECT <config_desktop_app_options>` - Select Bluetooth LE peer.
  This option can be used only by nRF Desktop peripheral.
* :ref:`CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST <config_desktop_app_options>` - Scan for new Bluetooth peers.
  This option can be used only by nRF Desktop central.

Peer control using a hardware selector
======================================

.. note::
    This feature can be used only by nRF Desktop peripheral devices.

Set the :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE <config_desktop_app_options>` option to use the dedicated local identity to connect with the dongle.
The last application local identity (the one with the highest ID) is used for this purpose.

The dongle is the nRF Desktop central.
If the dongle peer is enabled, the nRF Desktop peripheral uses one of the local identities for the Bluetooth connection with the dongle.
This local identity is meant to be paired with the dongle during the production process.

You can use the :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO <config_desktop_app_options>` option to indicate the dongle peer identity with the :c:struct:`ble_dongle_peer_event` event.

The dongle peer is selected using the :ref:`nrf_desktop_selector`.
You must also define the following parameters of the selector used to switch between dongle peer and other Bluetooth LE peers:

* :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_ID <config_desktop_app_options>` - Selector ID.
* :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_POS <config_desktop_app_options>` - Selector position for the dongle peer (when selector is in other position, other Bluetooth peers are selected).

.. note::
    The Bluetooth local identity used for the dongle peer does not provide any special capabilities.
    All the application modules and the Zephyr Bluetooth stack use it in the same way as any other local identity.
    The Low Latency Packet Mode can be used with any other local identity.

    The only difference is that selecting the dongle peer disables other peer operations.

Default Bluetooth local identity on peripheral
==============================================

By default, the default Bluetooth local identity is unused, because it cannot be reset.
You can set :ref:`CONFIG_DESKTOP_BLE_USE_DEFAULT_ID <config_desktop_app_options>` to make the nRF Desktop peripheral initially use the default Bluetooth local identity for the application local identity with ID ``0``.
After the successful erase advertising for application local identity with ID ``0``, the default Bluetooth local identity is switched out and it is no longer used.
The peer bonded with the default Bluetooth local identity is unpaired.

The default identity on given device uses the same Bluetooth address after every programming.
This feature can be useful for automatic tests.

Erasing dongle peers
====================

To enable erasing dongle peer you have to enable the following options:

* :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ERASE_BOND_BUTTON <config_desktop_app_options>` - If you want to enable erasing peers using buttons.
* :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ERASE_BOND_CONF_CHANNEL <config_desktop_app_options>` - If you want to enable erasing peers using `Configuration channel options`_.

Configuration channel options
*****************************

The module provides the following :ref:`nrf_desktop_config_channel` options:

* ``peer_erase`` - Erase peer for the current local identity.
  The nRF Desktop central erases all the peers.
  The nRF Desktop peripheral starts erase advertising.
* ``peer_search`` - Request scanning for new peripherals.
  The option is available only for the nRF Desktop central.

Perform a :ref:`nrf_desktop_config_channel` set operation on the selected option to trigger the operation.
The options can be used only if the module is in :c:enumerator:`STATE_IDLE`.
Because of this, they cannot be used when the device is suspended by :ref:`nrf_desktop_power_manager`.
The device must be woken up from suspended state before the operation is started.

Shell integration
*****************

The module is integrated with Zephyr's :ref:`zephyr:shell_api` module.
When the shell is turned on, an additional subcommand set (:command:`peers`) is added.

This subcommand set contains the following commands:

* :command:`show` - Show bonded peers for every Bluetooth local identity.
* :command:`remove` - Remove bonded peers for the currently selected Bluetooth local identity.
  The command instantly removes the peers for both nRF Desktop central and nRF Desktop peripheral.
  It does not start erase advertising.

Implementation details
**********************

The module uses Zephyr's :ref:`zephyr:settings_api` subsystem to store the following data in the non-volatile memory:

* Currently selected peer (application local identity)
* Mapping between the application local identities and the Bluetooth local identities
