.. _ble_latency:

BLE latency module
##################

The ``ble_latency`` module is used to:

* lower the BLE connection latency when :ref:`config_channel` is in use (low latency ensures quick data exchange).
* disconnect the connected Bluetooth Central if security was not established in predefined amount of time (``CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S``) after the connection occurred.

Module Events
*************

+--------------------+----------------------------------+-----------------+--------------+-------------+
| Source Module      | Input Event                      | This Module     | Output Event | Sink Module |
+====================+==================================+=================+==============+=============+
| ``ble_state``      | ``ble_peer_event``               | ``ble_latency`` |		 |	       |
+--------------------+----------------------------------+                 |              |	       |
| ``config``         |  ``config_event``                |                 |              |             |
+--------------------+                                  |                 |              |	       |
| ``config_channel`` |                                  |                 |              |	       |
+--------------------+----------------------------------+                 |              |	       |
| ``config_chanel``  |  ``config_fetch_request_event``  |                 |              |             |
+--------------------+----------------------------------+-----------------+--------------+-------------+

Configuration
*************

The module requires Bluetooth configuration - described in :ref:`bluetooth_guide`.
The module is enabled for every nRF52 Desktop Peripheral device (:option:`CONFIG_BT_PERIPHERAL`).

You can define maximum allowed time for connection security establishment (``CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S``).

Implementation details
**********************

The module uses the delayed works (:c:type:`struct k_delayed_work`) to control the connection latency and trigger the security timeout.
When :ref:`config_channel` is no longer in use (no :ref:`config_channel` events for :c:macro:`LOW_LATENCY_CHECK_PERIOD_MS`), the module sets the connection latency to :option:`CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY` to reduce device power consumption.
