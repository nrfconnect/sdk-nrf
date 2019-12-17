.. _ble_state:

BLE state module
################

Use the BLE state module to:

* enable Bluetooth (:cpp:func:`bt_enable`),
* handle Zephyr connection callbacks (:c:type:`struct bt_conn_cb`),
* handle Zephyr authenticated pairing callbacks (:c:type:`struct bt_conn_auth_cb`),
* propagate information about the connection state by using :ref:`event_manager` events.

Module Events
*************

+----------------+-------------------------+---------------+-----------------------+-----------------------+
| Source Module  | Input Event             | This Module   | Output Event          | Sink Module           |
+================+=========================+===============+=======================+=======================+
| ``ble_state``  | ``ble_peer_event``      | ``ble_state`` |                       |                       |
+----------------+-------------------------+               |                       |                       |
| :ref:`passkey` | ``passkey_input_event`` |               |                       |                       |
+----------------+-------------------------+               +-----------------------+-----------------------+
|                |                         |               | ``passkey_req_event`` | :ref:`passkey`        |
|                |                         |               +-----------------------+-----------------------+
|                |                         |               | ``ble_peer_event``    | :ref:`hids`           |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`led_state`      |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`ble_scan`       |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`ble_latency`    |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`ble_bond`       |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`ble_adv`        |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`power_manager`  |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`ble_discovery`  |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`hid_state`      |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | :ref:`hid_forward`    |
|                |                         |               |                       +-----------------------+
|                |                         |               |                       | ``ble_state``         |
+----------------+-------------------------+---------------+-----------------------+-----------------------+

Configuration
*************

The module requires the basic Bluetooth configuration, as described in the Bluetooth guide.

You can use the option ``CONFIG_DESKTOP_BLE_ENABLE_PASSKEY`` to enable pairing based on passkey for increased security.
Make sure to enable and configure the :ref:`passkey` module if you decide to use this option.

Implementation details
**********************

The ``ble_state`` module is used both by Bluetooth peripheral and central devices.

The module propagates information about the connection state changes using ``ble_peer_event``, where :cpp:member:`id` is a pointer to the connection object, and :cpp:member:`state` is the connection state.

The connection state can be set to one of the following values:

* :cpp:enum:`PEER_STATE_CONNECTED` - successfully connected to the remote peer.
* :cpp:enum:`PEER_STATE_CONN_FAILED` - failed to connect the remote peer.
* :cpp:enum:`PEER_STATE_SECURED` - set the connection security at least to level 2 (encryption and no authentication).
* :cpp:enum:`PEER_STATE_DISCONNECTED` - disconnected from the remote peer.

``ble_state`` module keeps references to :c:type:`struct bt_conn` objects to ensure that they remain valid when other application modules access them.
When a new connection is established, the module calls :cpp:func:`bt_conn_ref` to increase the object reference counter.
After ``ble_peer_event`` regarding disconnection or connection failure is received by all other application modules, ``ble_state`` module unreferences the :c:type:`struct bt_conn` object by using :cpp:func:`bt_conn_unref`.

For Bluetooth peripheral, the ``ble_state`` module is used to request the connection security level 2.
If the connection security level 2 is not established, the peripheral device disconnects.

Passkey enabled
   If you set the ``CONFIG_DESKTOP_BLE_ENABLE_PASSKEY`` option, the ``ble_state`` module registers the set of authenticated pairing callbacks (:c:type:`struct bt_conn_auth_cb`).
   The callbacks can be used to achieve higher security levels.
   The passkey input is handled in the :ref:`passkey` module.

Nrfxlib Link Layer
   If Nordic proprietary BLE Link Layer is selected (:option:`CONFIG_BT_LL_NRFXLIB`), the module sends a Bluetooth HCI command to enable the LLPM when Bluetooth is ready.
   The ``ble_state`` module also sets the TX power for connections, because Zephyr Kconfig options related to selecting the default TX power are not used by this Link Layer.
