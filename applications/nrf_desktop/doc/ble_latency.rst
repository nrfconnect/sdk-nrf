.. _nrf_desktop_ble_latency:

BLE latency module
##################

Use the BLE latency module to:

* Lower the BLE connection latency when :ref:`nrf_desktop_config_channel` is in use (low latency ensures a quick data exchange).
* Disconnect the Bluetooth Central if the connection has not been secured in the predefined amount of time after the connection occurred.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_latency_start
    :end-before: table_ble_latency_end

See the :ref:`nrf_desktop_architecture` for more information about the event-based communication in the nRF Desktop application and about how to read this table.

Configuration
*************

The module requires the basic Bluetooth configuration, as described in the Bluetooth guide.
The module is enabled for every nRF Desktop peripheral device.

You can use the option ``CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S`` to define the maximum allowed time for establishing the connection security.
If the connection is not secured during this period of time, the peripheral device disconnects.

Implementation details
**********************

The BLE latency module uses delayed works (:c:type:`struct k_delayed_work`) to control the connection latency and trigger the security timeout.

The module listens for ``config_event`` and ``config_fetch_request_event`` to detect when the :ref:`nrf_desktop_config_channel` is in use.

* When the :ref:`nrf_desktop_config_channel` is in use, the module sets the connection latency to low.
* When the :ref:`nrf_desktop_config_channel` is no longer in use (no :ref:`nrf_desktop_config_channel` events for ``LOW_LATENCY_CHECK_PERIOD_MS``), the module sets the connection latency to :option:`CONFIG_BT_PERIPHERAL_PREF_SLAVE_LATENCY` in order to reduce the power consumption.

The ``ble_latency`` module receives :ref:`nrf_desktop_config_channel` events, but it is not configurable with the :ref:`nrf_desktop_config_channel`.
The module does not register itself using the ``GEN_CONFIG_EVENT_HANDLERS`` macro.

If the nrfxlib Link Layer (:option:`CONFIG_BT_LL_NRFXLIB`) is selected on the connected Bluetooth central, the connection latency can be controlled only on the central side. (The peripheral's requests to modify the connection latency are ignored.)
