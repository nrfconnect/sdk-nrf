.. _nrf_desktop_factory_reset:

Factory reset module
####################

.. contents::
   :local:
   :depth: 2

The factory reset module allows to trigger device factory reset using a :ref:`nrf_desktop_config_channel` set operation.
The factory reset clears the stored Fast Pair data, removes Bluetooth bonds and resets all of the Bluetooth local identities, apart from the default one that cannot be reset.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_factory_reset_start
    :end-before: table_factory_reset_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The factory reset module requires enabling the following Kconfig options:

* :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE <config_desktop_app_options>` - The factory reset is triggered using a :ref:`nrf_desktop_config_channel` set operation.
* :kconfig:option:`CONFIG_BT_FAST_PAIR` - The factory reset is performed using the :ref:`bt_fast_pair_readme` API (:c:func:`bt_fast_pair_factory_reset`).
* :kconfig:option:`CONFIG_CAF_BLE_BOND_SUPPORTED` - The :ref:`nrf_desktop_ble_bond` must be ready before the factory reset is handled.

Use the :ref:`CONFIG_DESKTOP_FACTORY_RESET <config_desktop_app_options>` Kconfig option to enable the factory reset module.
The option is enabled by default if all of the required Kconfig options are enabled in configuration.

The :ref:`CONFIG_DESKTOP_FACTORY_RESET <config_desktop_app_options>` Kconfig option selects the following Kconfig options:

* :kconfig:option:`CONFIG_BT_FAST_PAIR_STORAGE_USER_RESET_ACTION` - The module uses the user reset action to remove Bluetooth bonds and reset Bluetooth local identities.
* :kconfig:option:`CONFIG_REBOOT` - The module triggers system reboot right after the factory reset operation is finished to ensure a proper device state after the factory reset.

Configuration channel options
*****************************

The module registers itself as ``factory_reset`` and provides the ``fast_pair`` option.
Perform set operation on the option to trigger the factory reset operation.

Implementation details
**********************

The factory reset is delayed by 250 ms to ensure that the host computer finalizes the configuration channel set operation.
