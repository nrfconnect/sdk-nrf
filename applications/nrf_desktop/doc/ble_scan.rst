.. _nrf_desktop_ble_scan:

Bluetooth LE scanning module
############################

.. contents::
   :local:
   :depth: 2

The nRF Desktop's |ble_scan| is based on the |NCS|'s :ref:`nrf_bt_scan_readme`.

Use the |ble_scan| to:

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

1. Configure Bluetooth, as described in :ref:`nrf_desktop_bluetooth_guide`.
#. Enable the following configuration options:

   * :option:`CONFIG_BT_SCAN`
   * :option:`CONFIG_BT_SCAN_FILTER_ENABLE`

   These options are used by the |NCS|'s :ref:`nrf_bt_scan_readme`.
#. Configure the number of scan filters based on the Bluetooth address (:option:`CONFIG_BT_SCAN_ADDRESS_CNT`).
   The value must be equal to the number of Bluetooth bonds.
   The number of Bluetooth bonds is defined by the :option:`CONFIG_BT_MAX_PAIRED` Kconfig option.
   The |ble_scan| module uses the Bluetooth address filters to look for bonded peripherals.
#. Configure the number of scan filters based on the Bluetooth name (:option:`CONFIG_BT_SCAN_NAME_CNT`).
   The |ble_scan| module uses Bluetooth name filters to look for unbonded peripherals.
   The value must be equal to the number of peripheral types the nRF Desktop central connects to.
   The peripheral type may be either a mouse or a keyboard.
#. If you want to limit the number of attempts to connect to a device, you can enable the connection attempt filter with the :option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER` Kconfig option.
   After the predefined number of disconnections or connection failures, the nRF Desktop central will no longer try to connect with the given peripheral device.
   This is done to prevent connecting and disconnecting with a peripheral in a never-ending loop.

   You can further configure this setting with the following Kconfig options:

   * :option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER_LEN` - This option defines the maximum number of filtered devices.
   * :option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_COUNT` - This option defines the connection attempt count for a given peripheral.

   The :ref:`nrf_bt_scan_readme` counts all disconnections for a peripheral.
   The |ble_scan| uses :c:func:`bt_scan_conn_attempts_filter_clear` to clear all the connection attempt counters on the following occasions:

   * After a successful peripheral discovery takes place (on ``ble_discovery_complete_event``).
   * When you request scan start or peer erase.

   If filters are not cleared by the application, the Bluetooth Central will be unable to reconnect to the peripheral after exceeding the maximum connection attempts.
#. Configure the maximum number of bonded mice (:option:`CONFIG_DESKTOP_BLE_SCAN_MOUSE_LIMIT`) and keyboards (:option:`CONFIG_DESKTOP_BLE_SCAN_KEYBOARD_LIMIT`) for the nRF Desktop central.
   By default, the nRF Desktop central connects and bonds with only one mouse and one keyboard.
#. Define the Bluetooth name filters in the :file:`ble_scan_def.h` file that is located in the board-specific directory in the application configuration directory.
   You must define a Bluetooth name filter for every peripheral type the nRF Desktop central connects to.
   For an example, see :file:`configuration/nrf52840dongle_nrf52840/ble_scan_def.h`.

   .. note::
      The Bluetooth device name for given peripheral is defined as the :option:`CONFIG_BT_DEVICE_NAME` Kconfig option in the peripheral's configuration.
      For more detailed information about the Bluetooth advertising configuration in the nRF Desktop application, see the :ref:`nrf_desktop_ble_adv` documentation.

#. Set the :option:`CONFIG_DESKTOP_BLE_SCANNING_ENABLE` option to enable the |ble_scan| module.

By default, the nRF Desktop central always looks for both bonded and unbonded peripherals.
You can set the :option:`CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST` option to make the device look for unbonded peripherals only on user request.
The request is submitted by :ref:`nrf_desktop_ble_bond` as :c:struct:`ble_peer_operation_event` with :c:member:`ble_peer_operation_event.op` set to :c:enumerator:`PEER_OPERATION_SCAN_REQUEST`.

The central always looks for new bonds also after the bond erase (on :c:struct:`ble_peer_operation_event` with :c:member:`ble_peer_operation_event.op` set to :c:enumerator:`PEER_OPERATION_ERASED`).
If :option:`CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST` is enabled, you can also set the :option:`CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_ON_BOOT` option to make the central scan for new peers after every boot.

The following scanning scenarios are possible:

* If no peripheral is connected, the central scans for the peripheral devices without interruption.
* If a peripheral is connected, the scanning is triggered periodically.
  If none of the connected peripherals is in use for at least :option:`CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S`, the scanning is started.

Scanning not started
====================

The scanning will not start if one of the following conditions occurs:

* There are no more free Bluetooth connections.
* The :ref:`nrf_desktop_ble_discovery` is in the process of discovering a peer.
* The central is going to scan only for bonded peers and all the bonded peers are already connected.

The number of Bluetooth connections is defined as the :option:`CONFIG_BT_MAX_CONN` Kconfig option.

Scanning interrupted
====================

The scanning is interrupted if one of the following conditions occurs:

* A connected peripheral is in use.
  Scanning in this situation will have a negative impact on user experience.
* The maximum scan duration specified by :option:`CONFIG_DESKTOP_BLE_SCAN_DURATION_S` times out.

The scanning is never interrupted if there is no connected Bluetooth peer.

Implementation details
**********************

The |ble_scan| module stores the following information for every bonded peer:

* Peripheral Bluetooth address.
* Peripheral type (mouse or keyboard).
* Information about Low Latency Packet Mode (LLPM) support.

The module uses Zephyr's :ref:`zephyr:settings_api` subsystem to store the information in the non-volatile memory.
This information is required to filter out unbonded devices, because the nRF Desktop central connects and bonds only with a defined number of mice and keyboards.

Bluetooth connection interval
=============================

After the scan filter match, the following happens:

a. The scanning is stopped and the |NCS|'s :ref:`nrf_bt_scan_readme` automatically establishes the Bluetooth connection with the peripheral.
   The initial Bluetooth connection interval is set by default to 7.5 ms, that is to the shortest connection interval allowed by the Bluetooth specification.
#. The connection is secured.
#. The peer discovery is started.
#. After the :ref:`nrf_desktop_ble_discovery` completes the peer discovery, the :ref:`nrf_desktop_ble_conn_params` receives the ``ble_discovery_complete_event`` and updates the Bluetooth connection interval.

.. important::
   If a Bluetooth peer is aready connected with a 1-ms connection interval, the next peer is connected with a 10-ms connection interval instead of 7.5 ms.
   The peer is connected with a 10-ms connection interval also in case :option:`CONFIG_BT_MAX_CONN` is set to value greater than 2 and :option:`CONFIG_DESKTOP_BLE_USE_LLPM` Kconfig option is enabled.
   This is required to avoid Bluetooth scheduling issues that may lead to HID input report rate drops and disconnections.

At this point, the scanning can be restarted.

.. |ble_scan| replace:: Bluetooth LE scanning module
