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

nRF Desktop uses the |ble_adv| from :ref:`lib_caf` (CAF).
See the :ref:`CAF Bluetooth LE advertising <caf_ble_adv>` page for implementation details.

For more information about Bluetooth configuration in nRF Desktop, see :ref:`nrf_desktop_bluetooth_guide`.

Avoiding connection requests from unbonded centrals when bonded
===============================================================

If the Bluetooth local identity currently in use already has a bond and the nRF Desktop device uses indirect advertising, the device will not set the General Discoverable flag.
The nRF Desktop devices also enable :kconfig:option:`CONFIG_BT_FILTER_ACCEPT_LIST` to mark incoming scan response data requests and connection requests as acceptable.
This is done to prevent Bluetooth Centrals other than the bonded one from connecting with the device.
The nRF Desktop dongle scans for peripheral devices using the Bluetooth device name, which is provided in the scan response data.

Transmission power level
========================

Bluetooth TX power level is assumed to be 0 dBm during advertising.
This value is sent in unbonded advertising.

.. |ble_adv| replace:: Bluetooth® LE advertising module
