.. _nrf_desktop_ble_bond:

Bluetooth LE bond module
########################

Use the |ble_bond| to manage the Bluetooth peers and bonds.
The module controls the following operations:

* Switching between peers (only for nRF Desktop peripheral)
* Triggering scanning for new peers (only for nRF Desktop central)
* Erasing the Bluetooth bonds

Module Events
*************

+-----------------------------------------------+--------------------------------+--------------+------------------------------+---------------------------------------------+
| Source Module                                 | Input Event                    | This Module  | Output Event                 | Sink Module                                 |
+===============================================+================================+==============+==============================+=============================================+
| :ref:`nrf_desktop_hids`                       | ``config_fetch_request_event`` | ``ble_bond`` |                              |                                             |
+-----------------------------------------------+                                |              |                              |                                             |
| :ref:`nrf_desktop_usb_state`                  |                                |              |                              |                                             |
+-----------------------------------------------+--------------------------------+              |                              |                                             |
| :ref:`nrf_desktop_selector`                   | ``selector_event``             |              |                              |                                             |
+-----------------------------------------------+--------------------------------+              |                              |                                             |
| :ref:`nrf_desktop_hids`                       | ``config_event``               |              |                              |                                             |
+-----------------------------------------------+                                |              |                              |                                             |
| :ref:`nrf_desktop_usb_state`                  |                                |              |                              |                                             |
+-----------------------------------------------+--------------------------------+              |                              |                                             |
| :ref:`nrf_desktop_ble_state`                  | ``ble_peer_event``             |              |                              |                                             |
+-----------------------------------------------+--------------------------------+              |                              |                                             |
| :ref:`nrf_desktop_click_detector`             | ``click_event``                |              |                              |                                             |
+-----------------------------------------------+--------------------------------+              +------------------------------+---------------------------------------------+
|                                               |                                |              | ``ble_peer_operation_event`` | :ref:`nrf_desktop_led_state`                |
|                                               |                                |              |                              +---------------------------------------------+
|                                               |                                |              |                              | :ref:`nrf_desktop_ble_scan`                 |
|                                               |                                |              |                              +---------------------------------------------+
|                                               |                                |              |                              | :ref:`nrf_desktop_ble_adv`                  |
|                                               |                                |              +------------------------------+---------------------------------------------+
|                                               |                                |              | ``config_fetch_event``       | :ref:`nrf_desktop_hids`                     |
|                                               |                                |              |                              +---------------------------------------------+
|                                               |                                |              |                              | :ref:`nrf_desktop_usb_state`                |
+-----------------------------------------------+--------------------------------+--------------+------------------------------+---------------------------------------------+

Local identities
****************

The nRF Desktop device uses the following types of local identities:

* Application local identity - Used by the application modules.
  The currently selected application local identity and its state are presented to the user by the :ref:`nrf_desktop_led_state`.
* Bluetooth local identity - Used by the :ref:`zephyr:bluetooth` API.
  Every Bluetooth local identity uses its own Bluetooth address and Identity Resolving Key (IRK).
  From the remote device's perspective, every Bluetooth local identity of a given peripheral is observed as a separate Bluetooth device.

Both application local identities and Bluetooth local identities are identified using IDs.
Both IDs related to the given Bluetooth peer operation are propagated in ``ble_peer_operation_event``.

The identity usage depends on the device type:

* nRF Desktop central
   The nRF Desktop central uses only one application local identity and only one Bluetooth local identity (the default ones).
* nRF Desktop peripheral
   The nRF Desktop peripheral uses multiple local identities.

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
Every transition is triggered by an :ref:`event_manager` event with a predefined value.
Some transitions can be also triggered by internal timeout.
For exmple transition from :cpp:enum:`STATE_ERASE_PEER` to :cpp:enum:`STATE_IDLE` can be triggered by ``click_event``, ``selector_event`` or internal timeout.

The following diagram shows states and transitions between these states after the module is initialized:

.. figure:: /images/nrf_desktop_ble_bond.svg
   :alt: nRF Desktop Bluetooth LE bond module state diagram

   nRF Desktop Bluetooth LE bond module state diagram (click to enlarge)

Receiving ``click_event`` with a click type that is not included in the schematic will result in cancelling the ongoing operation and returning to :cpp:enum:`STATE_IDLE`.
This does not apply to :cpp:enum:`STATE_DONGLE_CONN`.
In this state, all the peer operations triggered by ``click_event`` are disabled.

When the transition occurs:

* The ``ble_peer_operation_event`` with the defined :cpp:member:`op` is submitted.
  For example, when the user confirms the erase advertising, the ``ble_peer_operation_event`` is submitted with :cpp:member:`op` set to :cpp:enum:`PEER_OPERATION_ERASE_ADV`.
* The currently selected application local identity is updated (if anything changed).

Peer erasing
============

Depending on the device type:

* The nRF Desktop central erases all bonded peers on the erase confirmation.
* The nRF Desktop peripheral starts erase advertising on the erase confirmation.

Erase advertising
-----------------

The erase advertising is used to make sure that the user will be able to switch back to the old peer if the new one fails to connect or bond.
The peripheral uses an additional Bluetooth local identity that is not associated with any application local identity.
The peripheral resets the identity and starts advertising using it.

If a new peer successfully connects, establishes security, and bonds, the current application local identity switches to using the Bluetooth local identity that was used during the erase advertising.
The new peer is associated with the currently used application local identity.

After a timeout or on user request, the erase advertising is stopped.
The application local identity still uses the Bluetooth local identity that was associated with it before the erase advertising.

Configuration
*************

The module requires the basic Bluetooth configuration, as described in the Bluetooth guide.
The module is enabled for every nRF Desktop device.

You can control the connected peers using the following methods:

* `Peer control using a button`_
* `Peer control using a hardware selector`_

You can use both methods, but the hardware selector has precedence.

When configuring the module, you can also enable `Default Bluetooth local identity on peripheral`_.

Peer control using a button
===========================

Complete the following steps to let the user control Bluetooth peers using the dedicated button:

1. Set the ``CONFIG_DESKTOP_BLE_PEER_CONTROL`` option to enable the feature.
#. Configure the :ref:`nrf_desktop_buttons`.
#. Define the button's key ID as ``CONFIG_DESKTOP_BLE_PEER_CONTROL_BUTTON``.
#. Add the button to the :ref:`nrf_desktop_click_detector` configuration, because the |ble_bond| reacts on ``click_event``.

The following peer operations can be enabled:

* ``CONFIG_DESKTOP_BLE_PEER_ERASE`` - Bluetooth LE peer erase triggered at any time.
* ``CONFIG_DESKTOP_BLE_PEER_ERASE_ON_START`` - Erase advertising triggered by long press of the predefined button on system start.
  This option can be used only by nRF Desktop peripheral.
* ``CONFIG_DESKTOP_BLE_PEER_SELECT`` - Select Bluetooth LE peer.
  This option can be used only by nRF Desktop peripheral.
* ``CONFIG_DESKTOP0_BLE_NEW_PEER_SCAN_REQUEST`` - Scan for new Bluetooth peers.
  This option can be used only by nRF Desktop central.

Peer control using a hardware selector
======================================

.. note::
    This feature can be used only by nRF Desktop peripheral devices.

Set the ``CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE`` option to use the dedicated local identity to connect with the dongle.
The last application local identity (the one with the highest ID) is used for this purpose.

The dongle is the nRF Desktop central.
If the dongle peer is enabled, the nRF Desktop peripheral uses one of the local identities for the Bluetooth connection with the dongle.
This local identity is meant to be paired with the dongle during the production process.


The dongle peer is selected using the :ref:`nrf_desktop_selector`.
You must also define the following parameters of the selector used to switch between dongle peer and other Bluetooth LE peers:

* ``CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_ID`` - Selector ID.
* ``CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_POS`` - Selector position for the dongle peer (when selector is in other position, other Bluetooth peers are selected).

.. note::
    The Bluetooth local identity used for the dongle peer does not provide any special capabilities.
    All the application modules and the Zephyr Bluetooth stack use it in the same way as any other local identity.
    The Low Latency Packet Mode can be used with any other local identity.

    The only difference is that selecting the dongle peer disables other peer operations.

Default Bluetooth local identity on peripheral
==============================================

By default, the default Bluetooth local identity is unused, because it cannot be reset.
You can set ``CONFIG_DESKTOP_BLE_USE_DEFAULT_ID`` to make the nRF Desktop peripheral initially use the default Bluetooth local identity for the application local identity with ID ``0``.
After the successful erase advertising for application local identity with ID ``0``, the default Bluetooth local identity is switched out and it is no longer used.
The peer bonded with the default Bluetooth local identity is unpaired.

The default identity on given device uses the same Bluetooth address after every programming.
This feature can be useful for automatic tests.

Configuration channel
*********************

The module provides the following :ref:`nrf_desktop_config_channel` options:

* ``peer_erase`` - Erase peer for the current local identity.
  The nRF Desktop central erases all the peers.
  The nRF Desktop peripheral starts erase advertising.

* ``peer_search`` - Request scanning for new peripherals.
  The option is available only for the nRF Desktop central.

The options can be used only if the module is in :cpp:enum:`STATE_IDLE`.

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

.. |ble_bond| replace:: Bluetooth LE bond module
