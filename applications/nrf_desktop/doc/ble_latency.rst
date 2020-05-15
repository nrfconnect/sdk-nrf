.. _nrf_desktop_ble_latency:

Bluetooth LE latency module
###########################

Use the |ble_latency| to:

* Lower the Bluetooth LE connection latency when :ref:`nrf_desktop_config_channel` is in use (low latency ensures a quick data exchange).
* Keep the connection latency low for the LLPM (Low Latency Packet Mode) connections to improve performance.
* Disconnect the Bluetooth Central if the connection has not been secured in the predefined amount of time after the connection occurred.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_latency_start
    :end-before: table_ble_latency_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module requires the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
The module is enabled for every nRF Desktop peripheral device.

You can use the option ``CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S`` to define the maximum allowed time for establishing the connection security.
If the connection is not secured during this period of time, the peripheral device disconnects.

You can set the option ``CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK`` to keep the connection latency low for the LLPM connections.
This speeds up sending the first HID report after not sending a report for some connection intervals.
Enabling this option increases the power consumption - the connection latency is kept low unless the device is in the low power mode.
When the device is in the low power mode and :ref:`nrf_desktop_config_channel` is not used, the connection latency is set to higher value to reduce the power consumption.

Implementation details
**********************

The |ble_latency| uses delayed works (:c:type:`struct k_delayed_work`) to control the connection latency and trigger the security time-out.

The module listens for ``config_event`` and ``config_fetch_request_event`` to detect when the :ref:`nrf_desktop_config_channel` is in use.

* When the :ref:`nrf_desktop_config_channel` is in use, the module sets the connection latency to low.
* When the :ref:`nrf_desktop_config_channel` is no longer in use (no :ref:`nrf_desktop_config_channel` events for ``LOW_LATENCY_CHECK_PERIOD_MS``), the module sets the connection latency to :option:`CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY` in order to reduce the power consumption.

  .. note::
     If the option ``CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK`` is enabled, the LLPM connection latency is not increased unless the device is in the low power mode.

The ``ble_latency`` module receives :ref:`nrf_desktop_config_channel` events, but it is not configurable with the :ref:`nrf_desktop_config_channel`.
The module does not register itself using the ``GEN_CONFIG_EVENT_HANDLERS`` macro.

.. note::
   Zephyr's :ref:`zephyr:bluetooth` API does not allow to use the LLPM connection intervals in the connection parameter update request.
   If the LLPM connection interval is in use:

   * The nRF Desktop peripheral uses a 7.5-ms interval in the request.
   * The nRF Desktop central ignores the requested connection interval, and only the connection latency is updated.

   For more detailed information, see the :ref:`nrf_desktop_ble_conn_params` documentation page.

.. |ble_latency| replace:: Bluetooth LE latency module
