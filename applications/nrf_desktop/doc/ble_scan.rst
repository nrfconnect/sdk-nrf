.. _nrf_desktop_ble_scan:

Bluetooth LE scanning module
############################

.. contents::
   :local:
   :depth: 2

The nRF Desktop's Bluetooth® LE scanning module is based on the |NCS|'s :ref:`nrf_bt_scan_readme`.

Use the |ble_scan| for the following purposes:

* Apply scanning filters.
* Control the Bluetooth scanning.
* Initiate the Bluetooth connections.

This module can only be used by an nRF Desktop central.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_scan_start
    :end-before: table_ble_scan_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Complete the following steps to enable the |ble_scan|:

1. Complete the basic Bluetooth configuration, as described in the :ref:`nrf_desktop_bluetooth_guide` documentation.
   Make sure that both :ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` options are enabled.
   The |ble_scan| is enabled by the :ref:`CONFIG_DESKTOP_BLE_SCAN_ENABLE <config_desktop_app_options>` Kconfig option which is implied by :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` Kconfig option together with other application modules.
   These modules that are required for HID dongle that forwards the data from HID peripherals connected over Bluetooth.
#. Make sure that the number of scan filters based on the Bluetooth address (:kconfig:option:`CONFIG_BT_SCAN_ADDRESS_CNT`) is equal to the number of Bluetooth bonds.
   The number of Bluetooth bonds is defined by the :kconfig:option:`CONFIG_BT_MAX_PAIRED` Kconfig option.
   The |ble_scan| uses the Bluetooth address filters to look for bonded peripherals.
#. Make sure that the number of scan filters based on the Bluetooth name (:kconfig:option:`CONFIG_BT_SCAN_NAME_CNT`) is equal to the number of peripheral types the nRF Desktop central connects to.
   The |ble_scan| uses Bluetooth name filters to look for unbonded peripherals.
   The peripheral type may be either a mouse or a keyboard.
#. If you want to limit the number of attempts to connect to a device, you can use the connection attempt filter (:kconfig:option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER`).
   The Kconfig option is enabled by default.
   After the predefined number of disconnections or connection failures, the nRF Desktop central will no longer try to connect with the given peripheral device.
   This is done to prevent connecting and disconnecting with a peripheral in a never-ending loop.

   You can further configure this setting with the following Kconfig options:

   * :kconfig:option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER_LEN` - This option defines the maximum number of filtered devices.
   * :kconfig:option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_COUNT` - This option defines the connection attempt count for a given peripheral.

   The :ref:`nrf_bt_scan_readme` counts all disconnections for a peripheral.
   The |ble_scan| uses :c:func:`bt_scan_conn_attempts_filter_clear` to clear all the connection attempt counters on the following occasions:

   * After a successful peripheral discovery takes place (on ``ble_discovery_complete_event``).
   * When you request scan start or peer erase.

   If filters are not cleared by the application, the Bluetooth Central will be unable to reconnect to the peripheral after exceeding the maximum connection attempts.
#. Configure the maximum number of bonded mice (:ref:`CONFIG_DESKTOP_BLE_SCAN_MOUSE_LIMIT <config_desktop_app_options>`) and keyboards (:ref:`CONFIG_DESKTOP_BLE_SCAN_KEYBOARD_LIMIT <config_desktop_app_options>`) for the nRF Desktop central.
   By default, the nRF Desktop central connects and bonds with only one mouse and one keyboard.
#. Define the Bluetooth name filters in the :file:`ble_scan_def.h` file that is located in the board-specific directory in the application configuration directory.
   You must define a Bluetooth name filter for every peripheral type the nRF Desktop central connects to.
   For an example, see :file:`configuration/nrf52840dongle_nrf52840/ble_scan_def.h`.

   .. note::
      The Bluetooth device name for given peripheral is defined as the :kconfig:option:`CONFIG_BT_DEVICE_NAME` Kconfig option in the peripheral's configuration.
      For more detailed information about the Bluetooth advertising configuration in the nRF Desktop application, see the :ref:`nrf_desktop_ble_adv` documentation.

#. Set the :ref:`CONFIG_DESKTOP_BLE_SCAN_PM_EVENTS <config_desktop_app_options>` to block scanning in the power down mode to decrease the power consumption.
   The Kconfig option is enabled by default.

By default, the nRF Desktop central always looks for both bonded and unbonded peripherals.
You can set the :ref:`CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST <config_desktop_app_options>` option to make the device look for unbonded peripherals only on user request.
The request is submitted by :ref:`nrf_desktop_ble_bond` as :c:struct:`ble_peer_operation_event` with :c:member:`ble_peer_operation_event.op` set to :c:enumerator:`PEER_OPERATION_SCAN_REQUEST`.

The central always looks for new bonds also after the bond erase (on :c:struct:`ble_peer_operation_event` with :c:member:`ble_peer_operation_event.op` set to :c:enumerator:`PEER_OPERATION_ERASED`).
If :ref:`CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST <config_desktop_app_options>` is enabled, you can also set the :ref:`CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_ON_BOOT <config_desktop_app_options>` option to make the central scan for new peers after every boot.

The following scanning scenarios are possible:

* If no peripheral is connected, the central scans for the peripheral devices without interruption.
* If a peripheral is connected, the scanning is triggered periodically.
  If none of the connected peripherals is in use for at least :ref:`CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S <config_desktop_app_options>`, the scanning is started.

Scanning module configuration
=============================

The |ble_scan| relies on :ref:`nrf_bt_scan_readme` to perform Bluetooth scanning.
The |ble_scan| selects :kconfig:option:`CONFIG_BT_SCAN` and :kconfig:option:`CONFIG_BT_SCAN_FILTER_ENABLE`.
Apart from that the following default values are applied:

* :kconfig:option:`CONFIG_BT_SCAN_NAME_CNT` is set to ``2``.
  By default, nRF Desktop dongle connects to peripherals that are either keyboard or mouse.
* :kconfig:option:`CONFIG_BT_SCAN_ADDRESS_CNT` is set to :kconfig:option:`CONFIG_BT_MAX_PAIRED`.
  The dongle scans for all of the bonded peripherals.
* :kconfig:option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER` is enabled.

.. _nrf_desktop_ble_scan_scanning_not_started:

Scanning not started
====================

The scanning will not start if one of the following conditions occurs:

* There are no more free Bluetooth connections.
* The :ref:`nrf_desktop_ble_discovery` is in the process of discovering a peer.
* The central is going to scan only for bonded peers and all the bonded peers are already connected.
* The central is in the power down mode and :ref:`CONFIG_DESKTOP_BLE_SCAN_PM_EVENTS <config_desktop_app_options>` is enabled.

The number of Bluetooth connections is defined as the :kconfig:option:`CONFIG_BT_MAX_CONN` Kconfig option.

Scanning interrupted
====================

The scanning is interrupted if one of the following conditions occurs:

* A connected peripheral is in use.
  Scanning in this situation will have a negative impact on user experience.
* The maximum scan duration specified by :ref:`CONFIG_DESKTOP_BLE_SCAN_DURATION_S <config_desktop_app_options>` times out.

The scanning is never interrupted if there is no connected Bluetooth peer.
If :ref:`CONFIG_DESKTOP_BLE_SCAN_PM_EVENTS <config_desktop_app_options>` is enabled, the power down event will also interrupt scanning.

Forced scan
-----------

The module supports a dedicated forced scan state that prevents interrupting scanning when a connected peripheral is in use.
The forced scan speeds up establishing new connections with peripherals, but it also negatively impacts the performance of already connected peripherals.
If the :ref:`CONFIG_DESKTOP_BLE_FORCED_SCAN_DURATION_S <config_desktop_app_options>` Kconfig option is set to a value greater than zero, the module enters the forced scan state after one of the following conditions is met:

* Boot or wakeup
* Successful peripheral discovery
* Peripheral disconnection
* Connection failure
* Bond erase
* Scan request

After the duration that is specified in the :ref:`CONFIG_DESKTOP_BLE_FORCED_SCAN_DURATION_S <config_desktop_app_options>` Kconfig option, the module switches to regular scanning.
The regular scanning can be interrupted by using connected peripherals and times out after the scan duration specified by :ref:`CONFIG_DESKTOP_BLE_SCAN_DURATION_S <config_desktop_app_options>` if there are peripherals connected over Bluetooth.

.. note::
  The conditions described under :ref:`nrf_desktop_ble_scan_scanning_not_started` prevent the starting of the forced scan.

Implementation details
**********************

The |ble_scan| stores the following information for every bonded peer:

* Peripheral Bluetooth address.
* Peripheral type (mouse or keyboard).
* Information about Low Latency Packet Mode (LLPM) support.

The module uses Zephyr's :ref:`zephyr:settings_api` subsystem to store the information in the non-volatile memory.
This information is required to filter out unbonded devices, because the nRF Desktop central connects and bonds only with a defined number of mice and keyboards.

Bluetooth connection interval
=============================

After the scan filter match, the following happens:

1. The scanning is stopped and the |NCS|'s :ref:`nrf_bt_scan_readme` automatically establishes the Bluetooth connection with the peripheral.
   The initial Bluetooth connection interval is set by default to 7.5 ms, that is to the shortest connection interval allowed by the Bluetooth specification.
#. The peer discovery is started.
#. After the :ref:`nrf_desktop_ble_discovery` completes the peer discovery, the :ref:`nrf_desktop_ble_conn_params` receives the ``ble_discovery_complete_event`` and updates the Bluetooth connection interval.
   At this point, the scanning can be restarted.

.. important::
   If the dongle supports Low Latency Packet Mode (:kconfig:option:`CONFIG_CAF_BLE_USE_LLPM`) and more than one Bluetooth connection (:kconfig:option:`CONFIG_BT_MAX_CONN`), a 10-ms connection interval is used instead of 7.5 ms.
   This is done to avoid Bluetooth scheduling issues that may lead to HID input report rate drops and disconnections.
