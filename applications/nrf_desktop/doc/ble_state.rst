.. _nrf_desktop_ble_state:

Bluetooth LE state module
#########################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth LE state module to:

* Enable Bluetooth (:c:func:`bt_enable`).
* Handle Zephyr connection callbacks (:c:struct:`bt_conn_cb`).
* Handle Zephyr authenticated pairing callbacks (:c:struct:`bt_conn_auth_cb`).
* Propagate information about the connection state and parameters by using :ref:`event_manager` events.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_state_start
    :end-before: table_ble_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module requires the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.

You can use the option :option:`CONFIG_DESKTOP_BLE_ENABLE_PASSKEY` to enable pairing based on passkey for increased security.
Make sure to enable and configure the :ref:`nrf_desktop_passkey` if you decide to use this option.

Implementation details
**********************

The |ble_state| module is used by both Bluetooth Peripheral and Bluetooth Central devices.

Connection state change
=======================

The module propagates information about the connection state changes using :c:struct:`ble_peer_event`, where :c:member:`ble_peer_event.id` is a pointer to the connection object, and :c:member:`ble_peer_event.state` is the connection state.

The connection state can be set to one of the following values:

* :c:enum:`PEER_STATE_CONNECTED` - successfully connected to the remote peer.
* :c:enum:`PEER_STATE_CONN_FAILED` - failed to connect the remote peer.
* :c:enum:`PEER_STATE_SECURED` - set the connection security at least to level 2 (encryption and no authentication).
* :c:enum:`PEER_STATE_DISCONNECTED` - disconnected from the remote peer.

Connection parameters change
============================

The module submits a ``ble_peer_conn_params_event`` to inform other application modules about the following:

* Connection parameter update request.
* Connection parameter update.

The connection parameter update request is rejected in the Zephyr's callback.
The :ref:`nrf_desktop_ble_conn_params` updates the connection parameters on ``ble_peer_conn_params_event``.

Connection references
=====================

The |ble_state| keeps references to :c:struct:`bt_conn` objects to ensure that they remain valid when other application modules access them.
When a new connection is established, the module calls :c:func:`bt_conn_ref` to increase the object reference counter.
After ``ble_peer_event`` regarding disconnection or connection failure is received by all other application modules, the |ble_state| unreferences the :c:struct:`bt_conn` object by using :c:func:`bt_conn_unref`.

For Bluetooth Peripheral, the |ble_state| is used to request the connection security level 2.
If the connection security level 2 is not established, the peripheral device disconnects.

Passkey enabled
===============

If you set the :option:`CONFIG_DESKTOP_BLE_ENABLE_PASSKEY` option, the |ble_state| registers the set of authenticated pairing callbacks (:c:struct:`bt_conn_auth_cb`).
The callbacks can be used to achieve higher security levels.
The passkey input is handled in the :ref:`nrf_desktop_passkey`.

.. note::
    By default, Zephyr's Bluetooth Peripheral demands the security level 3 in case the passkey authentication is enabled.
    If the nRF Desktop dongle is unable to achieve the security level 3, it will be unable to connect with the peripheral.
    Disable the :option:`CONFIG_BT_SMP_ENFORCE_MITM` option to allow the dongle to connect without the authentication.

SoftDevice Link Layer
=====================

If Nordic Semiconductor's SoftDevice Bluetooth LE Link Layer is selected (:option:`CONFIG_BT_LL_SOFTDEVICE`) and the :option:`CONFIG_DESKTOP_BLE_USE_LLPM` option is enabled, the module sends a Bluetooth HCI command to enable the LLPM when Bluetooth is ready.
The |ble_state| also sets the TX power for connections, because Zephyr's Kconfig options related to selecting the default TX power are not used by this Link Layer.

.. |ble_state| replace:: Bluetooth LE state module
