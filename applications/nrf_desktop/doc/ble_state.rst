.. _ble_state:

BLE state module
################

The ``ble_state`` module is used to:

* enable Bluetooth (:cpp:func:`bt_enable`),
* handle Zephyr BLE connection callbacks (:c:type:`struct bt_conn_cb`),
* propagate information about the connection state using :ref:`event_manager` events.

Module Events
*************

+----------------+--------------------+---------------+--------------------+-----------------------+
| Source Module  | Input Event        | This Module   | Output Event       | Sink Module           |
+================+====================+===============+====================+=======================+
| ``ble_state``  | ``ble_peer_event`` | ``ble_state`` |			   |			   |
+----------------+--------------------+               +--------------------+-----------------------+
|                |                    |               | ``ble_peer_event`` | :ref:`hids`	   |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`led_state`      |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`ble_scan`       |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`ble_latency`    |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`ble_bond`       |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`ble_adv`        |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`power_manager`  |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`ble_discovery`  |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`hid_state`      |
|                |                    |               |                    +-----------------------+
|                |                    |               |                    | :ref:`hid_forward`    |
+----------------+--------------------+---------------+--------------------+-----------------------+

Configuration
*************

The module requires only basic Bluetooth configuration.
The configuration is described in separate file (work in progress).

Implementation details
**********************

The ``ble_state`` module is used both by Bluetooth Periperal and Central devices.

The module propagates information regarding the connection state changes using ``ble_peer_event``.
:cpp:member:`id` contains pointer to the connection object (:c:type:`struct bt_conn`).
:cpp:member:`state` contains information about the connection state.

Connection state can be set to one of the following values:

* :cpp:enum:`PEER_STATE_CONNECTED` - remote peer successfully connected.
* :cpp:enum:`PEER_STATE_CONN_FAILED` - failed to connect remote peer.
* :cpp:enum:`PEER_STATE_SECURED` - security for the connection set to level 2 (encryption and no authentication).
* :cpp:enum:`PEER_STATE_DISCONNECTED` - remote peer disconnected.

For Bluetooth Peripheral, the ``ble_state`` module is used to request connection security level 2.
If the connection security level 2 is not successfully established, the Peripheral device disconnects.

For Bluetooth Central, the module is used to initiate the MTU exchange (:cpp:func:`bt_gatt_exchange_mtu`).

If Nordic proprietary BLE Link Layer is selected (``CONFIG_BT_LL_NRFXLIB``), the module sends Bluetooth HCI command to enable LLPM when Bluetooth is ready.

``ble_state`` module keeps references to :c:type:`struct bt_conn` objects to ensure that they remain valid when other application modules access them.
When new connection is established, the module calls the :cpp:func:`bt_conn_ref` to increase object reference counter.
After ``ble_peer_event`` regarding disconnection is received by all other application modules, ``ble_state`` module unrefs the :c:type:`struct bt_conn` object using :cpp:func:`bt_conn_unref`.
