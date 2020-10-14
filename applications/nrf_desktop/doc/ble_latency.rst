.. _nrf_desktop_ble_latency:

Bluetooth LE latency module
###########################

.. contents::
   :local:
   :depth: 2

Use the |ble_latency| to:

* Lower the Bluetooth LE connection latency either when :ref:`nrf_desktop_config_channel` is in use or when a firmware update is received by the :ref:`nrf_desktop_smp` (low latency ensures quick data exchange).
* Request setting the initial connection parameters for a new Bluetooth connection.
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

You can use the option :option:`CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S` to define the maximum allowed time for establishing the connection security.
If the connection is not secured during this period of time, the peripheral device disconnects.

You can set the option :option:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK` to keep the connection latency low for the LLPM connections.
This speeds up sending the first HID report after not sending a report for some connection intervals.
Enabling this option increases the power consumption - the connection latency is kept low unless the device is in the low power mode.

Implementation details
**********************

The |ble_latency| uses delayed works (:c:struct:`k_delayed_work`) to control the connection latency and trigger the security time-out.

.. note::
   The module does not request an increase in the connection latency until the connection is secured.
   Increasing the slave latency can significantly increase the amount of time required to establish the Bluetooth connection security level on some hosts.

The module listens for the following events related to data transfer initiated by the connected Bluetooth central:

* ``config_event`` - This event is received when the :ref:`nrf_desktop_config_channel` is in use.
* ``ble_smp_transfer_event`` - This event is received when the firmware update is received by :ref:`nrf_desktop_smp`.

When these events are received, the module sets the connection latency to low.
When the :ref:`nrf_desktop_config_channel` is no longer in use and firmware update is not received by :ref:`nrf_desktop_smp` (no mentioned events for ``LOW_LATENCY_CHECK_PERIOD_MS``), the module sets the connection latency to :option:`CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY`  to reduce the power consumption.

  .. note::
     If the option :option:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK` is enabled, the LLPM connection latency is not increased unless the device is in the low power mode.

     When the device is in the low power mode and the events related to data transfer are not received, the connection latency is set to higher value to reduce the power consumption.

The ``ble_latency`` module receives :ref:`nrf_desktop_config_channel` events, but it is not configurable with the :ref:`nrf_desktop_config_channel`.
The module does not register itself using the ``GEN_CONFIG_EVENT_HANDLERS`` macro.

.. note::
   Zephyr's :ref:`zephyr:bluetooth` API does not allow to use the LLPM connection intervals in the connection parameter update request.
   If the LLPM connection interval is in use:

   * The nRF Desktop peripheral uses a 7.5-ms interval in the request.
   * The nRF Desktop central ignores the requested connection interval, and only the connection latency is updated.

   For more detailed information, see the :ref:`nrf_desktop_ble_conn_params` documentation page.

.. |ble_latency| replace:: Bluetooth LE latency module
