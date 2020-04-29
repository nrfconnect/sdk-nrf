.. _nrf_desktop_ble_scan:

Bluetooth LE scanning module
############################

The nRF Desktop's |ble_scan| is based on the |NCS|'s :ref:`nrf_bt_scan_readme`.

Use the |ble_scan| to:

* Apply scanning filters.
* Control the Bluetooth scanning.
* Initiate the Bluetooth connections.

This module can only be used by an nRF Desktop central.

Module Events
*************

+-----------------------------------------------+----------------------------------+--------------+---------------------------+---------------------------------------------+
| Source Module                                 | Input Event                      | This Module  | Output Event              | Sink Module                                 |
+===============================================+==================================+==============+===========================+=============================================+
| :ref:`nrf_desktop_ble_discovery`              | ``ble_discovery_complete_event`` | ``ble_scan`` |                           |                                             |
+-----------------------------------------------+----------------------------------+              |                           |                                             |
| :ref:`nrf_desktop_hid_state`                  | ``hid_report_event``             |              |                           |                                             |
+-----------------------------------------------+                                  |              |                           |                                             |
| :ref:`nrf_desktop_hid_forward`                |                                  |              |                           |                                             |
+-----------------------------------------------+----------------------------------+              |                           |                                             |
| :ref:`nrf_desktop_ble_bond`                   | ``ble_peer_operation_event``     |              |                           |                                             |
+-----------------------------------------------+----------------------------------+              |                           |                                             |
| :ref:`nrf_desktop_ble_state`                  | ``ble_peer_event``               |              |                           |                                             |
+-----------------------------------------------+----------------------------------+              +---------------------------+---------------------------------------------+
|                                               |                                  |              | ``ble_peer_search_event`` | :ref:`nrf_desktop_led_state`                |
+-----------------------------------------------+----------------------------------+--------------+---------------------------+---------------------------------------------+

Configuration
*************

Complete the following steps to enable the |ble_scan|:

1. Configure Bluetooth, as described in the nRF Desktop Bluetooth guide.
#. Enable the following configuration options:

   * :option:`CONFIG_BT_SCAN`,
   * :option:`CONFIG_BT_SCAN_FILTER_ENABLE`.

   These options are used by the |NCS|'s :ref:`nrf_bt_scan_readme`.
#. Configure the number of scan filters based on the Bluetooth address (:option:`CONFIG_BT_SCAN_ADDRESS_CNT`).
   The value must be equal to the number of Bluetooth bonds.
   The number of Bluetooth bonds is defined by the :option:`CONFIG_BT_MAX_PAIRED` Kconfig option.
   The |ble_scan| module uses the Bluetooth address filters to look for bonded peripherals.
#. Configure the number of scan filters based on the Bluetooth name (:option:`CONFIG_BT_SCAN_NAME_CNT`).
   The |ble_scan| module uses Bluetooth name filters to look for unbonded peripherals.
   The value must be equal to the number of peripheral types the nRF Desktop central connects to.
   The peripheral type may be either a mouse or a keyboard.
   The nRF Desktop central connects and bonds with only one peripheral of a given type.
#. Define the Bluetooth name filters in the :file:`ble_scan_def.h` file that is located in the board-specific directory in the application configuration folder.
   You must define a Bluetooth name filter for every peripheral type the nRF Desktop central connects to.
   For an example, see :file:`configuration/nrf52840dongle_nrf52840/ble_scan_def.h`.

   .. note::
      The Bluetooth device name for given peripheral is defined as the :option:`CONFIG_BT_DEVICE_NAME` Kconfig option in the peripheral's configuration.
      For more detailed information about the Bluetooth advertising configuration in the nRF Desktop application, see the :ref:`nrf_desktop_ble_adv` documentation.

#. Set the ``CONFIG_DESKTOP_BLE_SCANNING_ENABLE`` option to enable the |ble_scan| module.

By default, the nRF Desktop central always looks for both bonded and unbonded peripherals.
You can set the ``CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST`` option to make the device look for unbonded peripherals only on user request.
The request is submitted by :ref:`nrf_desktop_ble_bond` as ``ble_peer_operation_event`` with :cpp:member:`op` set to :cpp:enum:`PEER_OPERATION_SCAN_REQUEST`.

The central always looks for new bonds also after the bond erase (on ``ble_peer_operation_event`` with :cpp:member:`op` set to :cpp:enum:`PEER_OPERATION_ERASED`).
If ``CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST`` is enabled, you can also set the ``CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_ON_BOOT`` option to make the central scan for new peers after every boot.

Two scanning scenarios are possible:

* If no peripheral is connected, the central scans for the peripheral devices without interruption.
* If a peripheral is connected, the scanning is triggered periodically.
  If none of the connected peripherals is in use for at least ``CONFIG_DESKTOP_BLE_SCAN_START_TIMEOUT_S``, the scanning is started.

Scanning not started
    The scanning will not start if one of the following conditions occurs:

    * There are no more free Bluetooth connections.
    * The :ref:`nrf_desktop_ble_discovery` is in the process of discovering a peer.
    * The central is going to scan only for bonded peers and all the bonded peers are already connected.

    The number of Bluetooth connections is defined as the :option:`CONFIG_BT_MAX_CONN` Kconfig option.

Scanning interrupted
    The scanning is interrupted if one of the following conditions occurs:

    * A connected peripheral is in use.
      Scanning in this situation will have a negative impact on user experience.
    * The maximum scan duration specified by ``CONFIG_DESKTOP_BLE_SCAN_DURATION_S`` times out.

    The scanning continues even if there is no connected Bluetooth peer.

Implementation details
**********************

The |ble_scan| module stores the following information for every bonded peer:

* Peripheral Bluetooth address.
* Peripheral type (mouse or keyboard).
* Information about Low Latency Packet Mode (LLPM) support.

The module uses Zephyr's :ref:`zephyr:settings_api` subsystem to store the information in the non-volatile memory.
This information is required to filter out unbonded devices, because the nRF Desktop central connects and bonds with only one mouse and one keyboard.

Bluetooth connection inverval
   After the scan filter match, the following happens:

   1. The scanning is stopped and the |NCS|'s :ref:`nrf_bt_scan_readme` automatically establishes the Bluetooth connection with the peripheral.
      The initial Bluetooth connection interval is set by default to 7.5 ms, that is to the shortest connection interval allowed by the Bluetooth specification.
   #. The connection is secured.
   #. The peer discovery is started.
   #. After the :ref:`nrf_desktop_ble_discovery` completes the peer discovery, the |ble_scan| module receives the ``ble_discovery_complete_event``:

     * If the central and the connected peripheral both support the Low Latency Packet Mode (LLPM), the connection interval is set to 1 ms.
     * If neither the central or the connected peripheral support LLPM, or only one of them supports it, the interval is set to 7.5 ms.

   .. note::
      If a Bluetooth peer is aready connected with a 1-ms connection interval, the next peer is connected with a 10-ms connection interval instead of 7.5 ms.
      This is required to avoid Bluetooth scheduling issues.

   At this point, the scanning can be restarted.

Connection parameter update request
   The module handles Zephyr's Bluetooth LE connection parameter update request:

   * If the nrfxlib's Link Layer is selected (:option:`CONFIG_BT_LL_NRFXLIB`), the module rejects the requests.
   * If :option:`CONFIG_BT_LL_NRFXLIB` is not selected, the module sets the maximum and the minimum connection intervals to 7.5 ms, and then accepts the request.

.. |ble_scan| replace:: Bluetooth LE scanning module
