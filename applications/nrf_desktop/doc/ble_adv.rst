.. _nrf_desktop_ble_adv:

Bluetooth LE advertising module
###############################

.. contents::
   :local:
   :depth: 2

In nRF Desktop, the |ble_adv| is responsible for controlling the Bluetooth LE advertising for the nRF Desktop peripheral device.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_adv_start
    :end-before: table_ble_adv_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

nRF Desktop uses the |ble_adv| module from :ref:`lib_caf` (CAF).
See the :ref:`CAF Bluetooth LE advertising <caf_ble_adv>` page for implementation details.

For more information about Bluetooth configuration in nRF Desktop, see :ref:`nrf_desktop_bluetooth_guide`.

Avoiding connection requests from unbonded centrals when bonded
===============================================================

If the Bluetooth local identity currently in use already has a bond and the nRF Desktop device uses indirect advertising, the device will not set the General Discoverable flag.
The nRF desktop devices also enable :kconfig:`CONFIG_BT_WHITELIST` to whitelist incoming scan response data requests and connection requests.
This is done to prevent Bluetooth Centrals other than the bonded one from connecting with the device.
The nRF Desktop dongle scans for peripheral devices using the Bluetooth device name, which is provided in the scan response data.



.. |ble_adv| replace:: Bluetooth LE advertising module
