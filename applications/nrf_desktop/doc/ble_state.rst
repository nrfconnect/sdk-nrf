.. _nrf_desktop_ble_state:

Bluetooth LE state module
#########################

.. contents::
   :local:
   :depth: 2

In nRF Desktop, the Bluetooth® LE state module is responsible for the following actions:

* Enabling Bluetooth (:c:func:`bt_enable`).
* Handling Zephyr connection callbacks (:c:struct:`bt_conn_cb`).
* Propagating information about the connection state and parameters by using :ref:`app_event_manager` events.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_state_start
    :end-before: table_ble_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

nRF Desktop uses the Bluetooth LE state module from :ref:`lib_caf` (CAF).
The :ref:`CONFIG_DESKTOP_BLE_STATE <config_desktop_app_options>` Kconfig option selects the :kconfig:option:`CONFIG_CAF_BLE_STATE` option.

For more information about Bluetooth configuration in nRF Desktop, see :ref:`nrf_desktop_bluetooth_guide`.

Implementation details
**********************

The Bluetooth LE state module handles, among others, Zephyr's connection parameters update request callback.
The module rejects a connection parameters update request and submits the :c:struct:`ble_peer_conn_params_event` event.
The event is handled by the :ref:`nrf_desktop_ble_conn_params`, which updates the connection parameters.

For information on implementation details, see the :ref:`CAF Bluetooth LE state <caf_ble_state>` page.
