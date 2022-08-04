.. _nrf_desktop_ble_qos:

Bluetooth LE Quality of Service module
######################################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth® LE Quality of Service (QoS) module to achieve better connection quality and higher report rate by avoiding congested RF channels.
The module can be used by both nRF Desktop peripheral and nRF Desktop central with the SoftDevice Link Layer (:kconfig:option:`CONFIG_BT_LL_SOFTDEVICE`).
However, only the Bluetooth central can update the Bluetooth LE channel map that is in use.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_qos_start
    :end-before: table_ble_qos_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module requires the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.

The QoS module uses the ``chmap_filter`` library, whose API is described in :file:`src/util/chmap_filter/include/chmap_filter.h`.
The library is linked if :ref:`CONFIG_DESKTOP_BLE_QOS_ENABLE <config_desktop_app_options>` Kconfig option is enabled.

Enable the module using the :ref:`CONFIG_DESKTOP_BLE_QOS_ENABLE <config_desktop_app_options>` Kconfig option.
The option selects :kconfig:option:`CONFIG_BT_HCI_VS_EVT_USER`, because the module uses vendor-specific HCI events.

You can use the :ref:`CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE <config_desktop_app_options>` option to enable real-time QoS information printouts through a UART (e.g. a virtual COM port).
The chosen UART needs to be specified in Devicetree using ``ncs,ble-qos-uart``.
This option also enables and configures the COM port (USB CDC ACM).
For this reason, the :kconfig:option:`CONFIG_USB_DEVICE_STACK` must be enabled.

The QoS module creates additional thread for processing the QoS algorithm.
You can define the following options:

* :ref:`CONFIG_DESKTOP_BLE_QOS_INTERVAL <config_desktop_app_options>`
    This option specifies the amount of time of the processing interval for the QoS thread.
    The interval is defined in milliseconds.
    The thread periodically performs calculations and then sleeps during the interval.
    Longer intervals give more time to accumulate the Cyclic Redundancy Check (CRC) stats.
* :ref:`CONFIG_DESKTOP_BLE_QOS_STACK_SIZE <config_desktop_app_options>`
    This option defines the base stack size for the QoS thread.
* :ref:`CONFIG_DESKTOP_BLE_QOS_STATS_PRINT_STACK_SIZE <config_desktop_app_options>`
    This option specifies the stack size increase if :ref:`CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE <config_desktop_app_options>` is enabled.

.. tip::
   You can use the default thread stack sizes as long as you do not modify the module source code.

Configuration channel options
*****************************

You can use the :ref:`nrf_desktop_config_channel` to configure the module or read the configuration.
The module is a configuration channel listener and provides the following configuration options:

* ``sample_count_min``
   Minimum number of samples needed for channel map processing.
* ``min_channel_count``
   Minimum Bluetooth LE channel count.
* ``weight_crc_ok``
   Weight of CRC OK.
   Fixed point value with 1/100 scaling.
* ``weight_crc_error``
   Weight of CRC ERROR.
   Fixed point value with 1/100 scaling.
* ``ble_block_threshold``
   Threshold relative to the average rating for blocking Bluetooth LE channels.
   Fixed point value with 1/100 scaling.
* ``eval_max_count``
   Maximum number of blocked channels that can be evaluated.
* ``eval_duration``
   Duration of the channel evaluation (in seconds).
* ``eval_keepout_duration``
   Duration during which a channel will be blocked before it is considered for re-evaluation (in seconds).
* ``eval_success_threshold``
   Average rating threshold for approving a blocked Bluetooth LE channel that is under evaluation by the QoS module.
   Fixed point value with 1/100 scaling.
* ``wifi_rating_inc``
   Wi-Fi strength rating multiplier.
   Increase the value to block Wi-Fi faster.
   Fixed point value with 1/100 scaling.
* ``wifi_present_threshold``
   Average rating threshold for considering a Wi-Fi present.
   Fixed point with 1/100 scaling.
* ``wifi_active_threshold``
   Average rating threshold for considering a Wi-Fi active (blockable).
   Fixed point value with 1/100 scaling.
* ``channel_map``
   5-byte Bluetooth LE channel map bitmask.
   This option is read-only.
* ``wifi_blacklist``
   List of blacklisted Wi-Fi channels.
   The QoS module represents the list as a bitmask.
   In the :ref:`nrf_desktop_config_channel_script`, the Wi-Fi channels are provided in a comma-separated list, for example ``wifi_blacklist 1,3,5``.

Implementation details
**********************

The QoS module uses Zephyr's :ref:`zephyr:settings_api` subsystem to store the configuration in non-volatile memory.
The channel map is not stored.

SoftDevice Controller interaction
====================================

The module uses CRC information from the SoftDevice Controller to adjust the channel map.
The CRC information is received through the vendor-specific Bluetooth HCI event (:c:enum:`SDC_HCI_SUBEVENT_VS_QOS_CONN_EVENT_REPORT`).

Additional thread
=================

The module creates an additional low-priority thread.
The thread is used to periodically perform the following operations:

* Check and apply new configuration parameters received through the :ref:`nrf_desktop_config_channel`.
* Check and apply new blacklist received through the :ref:`nrf_desktop_config_channel`.
* Process channel map filter.
* Get channel map suggested by the ``chmap_filter`` library.
* Submit the suggested channel map as ``ble_qos_event``.
* If the device is a Bluetooth central, update the used Bluetooth LE channel map.

If the :ref:`CONFIG_DESKTOP_BLE_QOS_STATS_PRINTOUT_ENABLE <config_desktop_app_options>` Kconfig option is set, the module prints the following information through the virtual COM port:

* HID report rate
   The module counts the number of HID input reports received through Bluetooth LE and prints the report rate through the virtual COM port every 100 packets.
   The report rate is printed with a timestamp.

   Example output:

   .. code-block:: console

      [05399493]Rate:0455

* Bluetooth LE channel statistics
   The information is printed by the low-priority thread.
   The output (``BT_INFO``) consists of the Bluetooth LE channel information.
   Every Bluetooth LE channel information contains the following elements:

   * Last two digits of the channel frequency (2400 + x MHz)
   * Channel state
   * Channel rating

   If the channel map is updated, the ``Channel map update`` string is printed with a timestamp.

   Example output:

   .. code-block:: console

      [05407128]Channel map update
      BT_INFO={4:2:0,6:2:0,8:2:0,10:2:0,12:2:0,14:2:0,16:2:0,18:2:0,20:2:0,22:2:0,24:2:0,28:2:0,30:2:0,32:2:0,34:2:0,36:2:0,38:2:0,40:2:0,42:1:30,44:1:26,46:1:30,48:1:30,50:1:33,52:1:21,54:1:31,56:1:29,58:1:30,60:1:29,62:1:35,64:1:27,66:1:31,68:1:28,70:1:32,72:1:27,74:1:26,76:1:33,78:1:31,}
