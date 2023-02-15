.. _nrf_desktop_fast_pair_app:

Fast Pair module
################

.. contents::
   :local:
   :depth: 2

The Fast Pair module is used to:

* Update the Fast Pair advertising payload to automatically switch between showing and hiding user interface (UI) pairing indication on the Fast Pair Seeker.
  The UI indication must be displayed only if the Provider can bond with new peers on the currently used Bluetooth local identity.
* Reject a normal Bluetooth pairing when outside of the pairing mode.
* Remove the Fast Pair advertising payload for the dongle peer.

The module is used when integrating Google `Fast Pair`_ to nRF Desktop application.
See :ref:`nrf_desktop_bluetooth_guide_fast_pair` section of the nRF Desktop documentation for detailed information about Fast Pair integration in the application.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_fast_pair_app_start
    :end-before: table_fast_pair_app_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The Fast Pair module requires enabling the following Kconfig options:

* :kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR` - The nRF Desktop's :ref:`nrf_desktop_ble_adv` uses :ref:`bt_le_adv_prov_readme` to generate advertising and scan response data.
  The Google Fast Pair advertising data provider (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR`) is used to add the Fast Pair payload to the advertising data.
  The Fast Pair module uses API of the Google Fast Pair advertising data provider to switch between showing and hiding the UI indication.
  The UI indication is displayed only if the Provider can bond with new peers on the currently used Bluetooth local identity.
  The Provider supports up to :kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS` bonds per Bluetooth local identity.
* :kconfig:option:`CONFIG_CAF_BLE_COMMON_EVENTS` - The module updates the Fast Pair advertising payload reacting on the application events related to Bluetooth.
  See :ref:`nrf_desktop_bluetooth_guide` for Bluetooth configuration in the nRF Desktop.

The Fast Pair module is enabled using :ref:`CONFIG_DESKTOP_FAST_PAIR <config_desktop_app_options>` Kconfig option.
The option is enabled by default if :kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS` Kconfig option value is greater than one.

.. note::
   If :kconfig:option:`CONFIG_CAF_BLE_STATE_MAX_LOCAL_ID_BONDS` Kconfig option value is equal to one:

   * Displaying UI indication during the Fast Pair not discoverable advertising (:kconfig:option:`CONFIG_BT_ADV_PROV_FAST_PAIR_SHOW_UI_PAIRING`) is disabled by default in the nRF Desktop advertising data configuration defined in :file:`src/util/Kconfg` file.
   * :ref:`nrf_desktop_ble_state` automatically disconnects new peers right after Bluetooth connection is established if the used Bluetooth local identity is already bonded with another peer.

The :ref:`CONFIG_DESKTOP_FAST_PAIR_LIMIT_NORMAL_PAIRING <config_desktop_app_options>` can be used to allow normal Bluetooth pairing only in the pairing mode.
Normal Bluetooth pairing is rejected when outside of the pairing mode (if the used Bluetooth local identity already has a bonded peer).
The option is enabled by default.

With the dongle peer functionality enabled (:ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE <config_desktop_app_options>`), the Fast Pair module selects the :ref:`CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO <config_desktop_app_options>` option to track the application identity of the dongle peer.

Implementation details
**********************

The module is an early subscriber for :c:struct:`ble_peer_event` and :c:struct:`ble_peer_operation_event`.
This allows the module to update the Fast Pair advertising payload just before the Bluetooth advertising is started.

The module is a subscriber for :c:struct:`ble_dongle_peer_event`.
This allows the module to remove the Fast Pair advertising payload when the application identity of the dongle peer is used.

The module registers the global application's Bluetooth authentication callbacks (:c:struct:`bt_conn_auth_cb`) on application start.
The callbacks are used to reject normal Bluetooth pairing when outside of the pairing mode.
