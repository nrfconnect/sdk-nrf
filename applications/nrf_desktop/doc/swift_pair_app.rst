.. _nrf_desktop_swift_pair_app:

Swift Pair module
#################

.. contents::
   :local:
   :depth: 2

The Swift Pair module is used to enable or disable the Swift Pair Bluetooth advertising payload depending on the selected Bluetooth peer (used local identity).
The module distinguishes between the dongle peer and the general Bluetooth peers.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_swift_pair_app_start
    :end-before: table_swift_pair_app_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To use the Swift Pair module, you must enable the following Kconfig options:

* :kconfig:option:`CONFIG_BT_ADV_PROV_SWIFT_PAIR` - The nRF Desktop's :ref:`nrf_desktop_ble_adv` uses the :ref:`bt_le_adv_prov_readme` to generate advertising and scan response data.
  The Swift Pair advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_SWIFT_PAIR`) is used to add the Swift Pair payload to the advertising data.
  The Swift Pair module uses API of the Swift Pair advertising data provider and depending on the application local identity it enables or disables the Swift Pair advertising payload.
* :kconfig:option:`CONFIG_CAF_BLE_COMMON_EVENTS` - The module updates the Fast Pair advertising payload by reacting on the application events related to Bluetooth.
  See :ref:`nrf_desktop_bluetooth_guide` for Bluetooth configuration in the nRF Desktop.
* :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE <config_desktop_app_options>` - The module dynamically enables or disables the Swift Pair advertising payload for the dongle peer and for other Bluetooth peers relying on the values of the following Kconfig options:

  * :ref:`CONFIG_DESKTOP_SWIFT_PAIR_ADV_DONGLE_PEER <config_desktop_app_options>`
  * :ref:`CONFIG_DESKTOP_SWIFT_PAIR_ADV_GENERAL_PEER <config_desktop_app_options>`

  If the dongle peer is disabled, there is no reason to use the module.
  The Swift Pair advertising data provider can be simply enabled or disabled during a build time through a dedicated Kconfig option.

Set the :ref:`CONFIG_DESKTOP_SWIFT_PAIR <config_desktop_app_options>` Kconfig option to enable the module.
This option automatically selects the :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO <config_desktop_app_options>` Kconfig option to enable the dongle peer ID information event.

Implementation details
**********************

The module is an early subscriber for :c:struct:`ble_peer_operation_event`.
This allows the module to enable or disable the Swift Pair advertising payload just before the Bluetooth advertising starts.

The module is a subscriber for :c:struct:`ble_dongle_peer_event`.
This allows the module to track the application local identity of the dongle peer.
