.. _nrf_desktop_ble_latency:

Bluetooth LE latency module
###########################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth® LE latency module for the following purposes:

* Lower the Bluetooth LE connection latency when the :ref:`nrf_desktop_config_channel` is in use or when a firmware update is received either by the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` (low latency ensures quick data exchange).
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
Make sure that both :ref:`CONFIG_DESKTOP_ROLE_HID_PERIPHERAL <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>` options are enabled.
The Bluetooth LE latency application module is enabled by the :ref:`CONFIG_DESKTOP_BLE_LATENCY_ENABLE <config_desktop_app_options>` option.
The option is implied by :ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>` together with other features used by a HID peripheral device.

You can use the option :ref:`CONFIG_DESKTOP_BLE_SECURITY_FAIL_TIMEOUT_S <config_desktop_app_options>` to define the maximum allowed time for establishing the connection security.
If the connection is not secured during this period of time, the peripheral device disconnects.

You can set the option :ref:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK <config_desktop_app_options>` to keep the connection latency low for the LLPM connections.
This speeds up sending the first HID report after not sending a report for some connection intervals.
Enabling this option increases the power consumption - the connection latency is kept low unless the device is in the low power mode.

You can use the :ref:`CONFIG_DESKTOP_BLE_LATENCY_PM_EVENTS <config_desktop_app_options>` Kconfig option to enable or disable handling of the power management events, such as :c:struct:`power_down_event` and :c:struct:`wake_up_event`.
The option is enabled by default and depends on the :kconfig:option:`CONFIG_CAF_PM_EVENTS` Kconfig option.

Implementation details
**********************

The |ble_latency| uses delayed works (:c:struct:`k_work_delayable`) to control the connection latency and trigger the security timeout.

.. note::
   The module does not request an increase in the connection latency until the connection is secured.
   Increasing the slave latency can significantly increase the amount of time required to establish the Bluetooth connection security level on some hosts.

The module listens for the following events related to data transfer initiated by the connected Bluetooth central:

* ``config_event`` - This event is received when the :ref:`nrf_desktop_config_channel` is in use.
* ``ble_smp_transfer_event`` - This event is received when either the :ref:`nrf_desktop_ble_smp` or :ref:`nrf_desktop_dfu_mcumgr` receives a firmware update.

When these events are received, the module sets the connection latency to low.
When the :ref:`nrf_desktop_config_channel` is no longer in use, and neither :ref:`nrf_desktop_ble_smp` nor :ref:`nrf_desktop_dfu_mcumgr` receive firmware updates (no mentioned events for ``LOW_LATENCY_CHECK_PERIOD_MS``), the module sets the connection latency to :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_LATENCY` to reduce the power consumption.

  .. note::
     If the option :ref:`CONFIG_DESKTOP_BLE_LOW_LATENCY_LOCK <config_desktop_app_options>` is enabled, the LLPM connection latency is not increased unless the device is in the low power mode.

     When the device is in the low power mode and the events related to data transfer are not received, the connection latency is set to higher value to reduce the power consumption.

The ``ble_latency`` module receives :ref:`nrf_desktop_config_channel` events, but it is not configurable with the :ref:`nrf_desktop_config_channel`.
The module does not register itself using the ``GEN_CONFIG_EVENT_HANDLERS`` macro.

.. note::
   Zephyr's :ref:`zephyr:bluetooth` API does not allow to use the LLPM connection intervals in the connection parameter update request.
   If the LLPM connection interval is in use:

   * The nRF Desktop peripheral uses a 7.5-ms interval in the request.
   * The nRF Desktop central ignores the requested connection interval, and only the connection latency is updated.

   For more detailed information, see the :ref:`nrf_desktop_ble_conn_params` documentation page.
