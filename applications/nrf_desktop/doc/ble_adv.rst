.. _nrf_desktop_ble_adv:

Bluetooth LE advertising module
###############################

Use the Bluetooth LE advertising module to control the Bluetooth LE advertising for the nRF Desktop peripheral device.

Module Events
*************

+------------------------------+------------------------------+-------------+---------------------------+------------------------------+
| Source Module                | Input Event                  | This Module | Output Event              | Sink Module                  |
+==============================+==============================+=============+===========================+==============================+
| :ref:`nrf_desktop_ble_state` | ``ble_peer_event``           | ``ble_adv`` |                           |                              |
+------------------------------+------------------------------+             |                           |                              |
| :ref:`nrf_desktop_ble_bond`  | ``ble_peer_operation_event`` |             |                           |                              |
+------------------------------+------------------------------+             +---------------------------+------------------------------+
|                              |                              |             | ``ble_peer_search_event`` | :ref:`nrf_desktop_led_state` |
+------------------------------+------------------------------+-------------+---------------------------+------------------------------+


Configuration
*************

Complete the following steps in order to enable the Bluetooth LE advertising module:

1. Configure Bluetooth, as described in the Bluetooth guide.
#. Define the short device name used for advertising (``CONFIG_DESKTOP_BLE_SHORT_NAME``).
#. Define the Bluetooth device name (:option:`CONFIG_BT_DEVICE_NAME`).
#. Define the Bluetooth device appearance (:option:`CONFIG_BT_DEVICE_APPEARANCE`).
#. Set the ``CONFIG_DESKTOP_BLE_ADVERTISING_ENABLE`` Kconfig option.

Using directed advertising
    By default, the module uses indirect advertising.
    Set the ``CONFIG_DESKTOP_BLE_DIRECT_ADV`` option to use directed advertising.
    The directed advertising can be used to call the selected peer device to connect as quickly as possible.

    .. note::
       The nRF Desktop peripheral will not advertise directly towards a central that uses Resolvable Private Address (RPA).
       The peripheral does not read the Central Address Resolution GATT characteristic of the central, so the peripheral does not know if the remote device supports the address resolution of directed advertisements.

Changing advertising interval
    Set the ``CONFIG_DESKTOP_BLE_FAST_ADV`` Kconfig option to make the peripheral initially advertise with shorter interval in order to speed up finding the peripheral by Bluetooth centrals.

    * If the device uses indirect advertising, it will switch to slower advertising after the period of time defined in ``CONFIG_DESKTOP_BLE_FAST_ADV_TIMEOUT`` (in seconds).
    * If the device uses directed advertising, the ``ble_adv`` module will receive ``ble_peer_event`` with :cpp:member:`state` set to :cpp:enumerator:`PEER_STATE_CONN_FAILED` if the central does not connect during the predefined period of fast directed advertising.
      After the event is received, the device will switch to the low duty cycle directed avertising.

    Switching to slower advertising is done to reduce the energy consumption.

Using Swift Pair
    You can use the ``CONFIG_DESKTOP_BLE_SWIFT_PAIR`` option to enable the Swift Pair feature.
    The feature simplifies pairing the Bluetooth peripheral with Windows 10 hosts.

Power down
    When the system goes to the Power Down state, the advertising stops.

    If the Swift Pair feature is enabled, the device advertises without the Swift Pair data for additional ``CONFIG_DESKTOP_BLE_SWIFT_PAIR_GRACE_PERIOD`` seconds to ensure that the user does not try to connect to the device that is no longer available.

Implementation details
**********************

The BLE advertising module uses the :ref:`zephyr:settings_api` to store the information if the peer for the given local identity uses the Resolvable Private Address (RPA).

Avoiding connection requests from unbonded centrals when bonded
    If the Bluetooth local identity currently in use already has a bond and the device uses indirect advertising, the advertising device will not set the General Discoverable flag.
    If :option:`CONFIG_BT_WHITELIST` is enabled, the device will also whitelist incoming scan response data requests and connection requests.
    This is done to prevent Bluetooth centrals other than the bonded one from connecting with the device.
    The nRF Desktop dongle scans for peripheral devices using the Bluetooth device name, which is provided in the scan response data.
