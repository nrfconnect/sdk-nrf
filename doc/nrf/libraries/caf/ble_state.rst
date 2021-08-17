.. _caf_ble_state:

CAF: Bluetooth LE state module
##############################

.. contents::
   :local:
   :depth: 2

The |ble_state| is a core Bluetooth module in :ref:`lib_caf` (CAF).
When enabled for an application, the |ble_state| is responsible for the following actions:

* Enabling Bluetooth (:c:func:`bt_enable`)
* Handling Bluetooth connection callbacks (:c:struct:`bt_conn_cb`)
* Propagating information about the connection state and parameters with :ref:`event_manager` events

The |ble_state| does not handle Bluetooth advertising or scanning.
If you want to use these functionalities to connect over Bluetooth LE, use :ref:`CAF's Bluetooth LE advertising module <caf_ble_adv>` or Zephyr's :ref:`Bluetooth API <zephyr:bluetooth_api>` directly.

.. note::
   CAF assumes that Bluetooth Peripheral device supports only one simultaneous connection and can have up to one bond per Bluetooth local identity.

Configuration
*************

To use the module, you must enable the following Kconfig options:

* :option:`CONFIG_BT`
* :option:`CONFIG_BT_SMP` - This option enables Security Manager Protocol support.
  The |ble_state| on Bluetooth Peripheral requires at least the connection security level 2 (encryption).
* :option:`CONFIG_CAF_BLE_STATE` - This option enables the |ble_state| and selects the :option:`CONFIG_CAF_BLE_COMMON_EVENTS` Kconfig option, which enables Bluetooth LE common events in CAF.

The following Kconfig options are also available for this module:

* :option:`CONFIG_CAF_BLE_STATE_EXCHANGE_MTU` - This option can be used for GATT client (:option:`CONFIG_BT_GATT_CLIENT`) to set the Maximum Transmission Unit (MTU) to the maximum possible size that the buffers can hold.
  This option is enabled by default.
* :option:`CONFIG_CAF_BLE_USE_LLPM` - This option enables the Low Latency Packet Mode (LLPM).
  This option is enabled by default and depends on :option:`CONFIG_BT_CTLR_LLPM` and :option:`CONFIG_BT_LL_SOFTDEVICE`.

Implementation details
**********************

The |ble_state| is used by both Bluetooth Peripheral and Bluetooth Central devices.

In line with other CAF modules, the |ble_state| uses :ref:`event_manager` events to broadcast changes in connection state and parameters.
It also updates connection reference counts to ensure the connections remain valid as long as application modules use them.

Connection state change
=======================

The module propagates information about the connection state changes using :c:struct:`ble_peer_event`.
In this event, :c:member:`ble_peer_event.id` is a pointer to the connection object and :c:member:`ble_peer_event.state` is the connection state.

.. figure:: images/caf_ble_state_transitions.svg
   :alt: Bluetooth connection state handling in CAF

   Bluetooth connection state handling in CAF

The connection state can be set to one of the following values:

* :c:enum:`PEER_STATE_CONNECTED` - Bluetooth stack successfully connected to the remote peer.
* :c:enum:`PEER_STATE_CONN_FAILED` - Bluetooth stack failed to connect the remote peer.
* :c:enum:`PEER_STATE_SECURED` - Bluetooth stack set the connection security to at least level 2 (that is, encryption and no authentication).
* :c:enum:`PEER_STATE_DISCONNECTED` - Bluetooth stack disconnected from the remote peer.

Other application modules can call :c:func:`bt_conn_disconnect` to disconnect the remote peer.
The application module can submit a :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enum:`PEER_STATE_DISCONNECTING` to let other application modules prepare for the disconnection.

On Bluetooth Peripheral, the |ble_state| requires the connection security level 2.
If the connection security level 2 is not established, the Peripheral disconnects.

Connection parameter change
===========================

The module submits a :c:struct:`ble_peer_conn_params_event` to inform other application modules about connection parameter update requests and connection parameter updates.

The |ble_state| rejects the connection parameter update request in Zephyr's callback.
An application module can handle the :c:struct:`ble_peer_conn_params_event` and update the connection parameters.

Connection references
=====================

The |ble_state| keeps references to :c:struct:`bt_conn` objects to ensure that they remain valid when other application modules access them.
When a new connection is established, the module calls :c:func:`bt_conn_ref` to increase the object reference counter.
After :c:struct:`ble_peer_event` about disconnection or connection failure is received by all other application modules, the |ble_state| decrements the :c:struct:`bt_conn` object by using :c:func:`bt_conn_unref`.

Behavior with SoftDevice Link Layer
===================================

If Nordic Semiconductor's SoftDevice Bluetooth LE Link Layer is selected (:option:`CONFIG_BT_LL_SOFTDEVICE`) and the :option:`CONFIG_CAF_BLE_USE_LLPM` option is enabled, the |ble_state| sends a Bluetooth HCI command to enable the LLPM when Bluetooth is ready.

If the SoftDevice Link Layer is selected, the |ble_state| also sets the TX power for connections.
The TX power is set according to Zephyr's Kconfig options related to selecting the default TX power.
This is necessary because the mentioned Kconfig options are not automatically applied by the Bluetooth stack if the SoftDevice Link Layer is selected.

.. |ble_state| replace:: Bluetooth LE state module
