.. _nrf_desktop_ble_passkey:

Bluetooth LE passkey module
###########################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth LE passkey module to enable pairing based on passkey for increased security.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_passkey_start
    :end-before: table_ble_passkey_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module requires the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
The module can be used only for nRF Desktop Bluetooth Peripheral devices (:ref:`CONFIG_DESKTOP_BT_PERIPHERAL <config_desktop_app_options>`).

Use the option :ref:`CONFIG_DESKTOP_BLE_ENABLE_PASSKEY <config_desktop_app_options>` to enable the module.
Make sure to enable and configure the :ref:`nrf_desktop_passkey` if you decide to use this option.

Implementation details
**********************

The Bluetooth LE passkey module is used only by Bluetooth Peripheral devices.

Passkey enabled
===============

The Bluetooth LE passkey module registers the set of authenticated pairing callbacks (:c:struct:`bt_conn_auth_cb`).
The callbacks can be used to achieve higher security levels.
The passkey input is handled in the :ref:`nrf_desktop_passkey`.

.. note::
    By default, Zephyr's Bluetooth Peripheral demands the security level 3 in case the passkey authentication is enabled.
    If the nRF Desktop dongle is unable to achieve the security level 3, it will be unable to connect with the peripheral.
    The :kconfig:option:`CONFIG_BT_SMP_ENFORCE_MITM` option is disabled by default to allow the dongle to connect without the authentication.
