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

Configuration
*************

nRF Desktop uses the |ble_adv| from :ref:`lib_caf` (CAF).
The :ref:`CONFIG_DESKTOP_BLE_ADV <config_desktop_app_options>` Kconfig option selects :kconfig:option:`CONFIG_CAF_BLE_ADV` and aligns the default module configuration to the application requirements.
For details on the default configuration alignment, see the following sections.

For more information about Bluetooth configuration in nRF Desktop, see :ref:`nrf_desktop_bluetooth_guide`.

Avoiding connection requests from unbonded centrals when bonded
===============================================================

If the Bluetooth local identity currently in use already has a bond and the nRF Desktop device uses indirect advertising, the device will not set the General Discoverable flag.
The nRF Desktop devices also enable :kconfig:option:`CONFIG_CAF_BLE_ADV_FILTER_ACCEPT_LIST` to filter incoming scan response data requests and connection requests.
This is done to prevent Bluetooth Centrals other than the bonded one from connecting with the device.
The nRF Desktop dongle scans for peripheral devices using the Bluetooth device name, which is provided in the scan response data.

Advertised data
===============

The :ref:`caf_ble_adv` relies on :ref:`bt_le_adv_prov_readme` to manage advertising data and scan response data.
nRF Desktop application configures the data providers in :file:`src/modules/Kconfig.caf_ble_adv.default`.
By default, the application enables a set of data providers available in the |NCS| and adds a custom provider of UUID16 values of Battery Service (BAS) and Human Interface Device Service (HIDS).
The UUID16 of a given GATT Service is added to the advertising data only if the service is enabled in the configuration and the Bluetooth local identity in use has no bond.

Implementation details
**********************

See the :ref:`CAF Bluetooth LE advertising <caf_ble_adv>` page for implementation details.
