.. _caf_ble_state_pm:

CAF: Bluetooth state power manager module
#########################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® state power manager module is a minor module that counts the number of active Bluetooth connections and imposes a :ref:`power manager module <caf_power_manager>` power level restriction if there is at least one active connection.

Configuration
*************

To enable the module, set the :kconfig:option:`CONFIG_CAF_BLE_STATE_PM` Kconfig option.
It depends on the :kconfig:option:`CONFIG_CAF_BLE_STATE` and :kconfig:option:`CONFIG_CAF_POWER_MANAGER` options.

Implementation details
**********************

The module reacts only to :c:struct:`ble_peer_event`.
Upon the reception of the event, the module checks if the Bluetooth peer is connected or disconnected.
It then counts the active connections.

Depending on the count result:

* If there is at least one active connection, the power level is limited to :c:enum:`POWER_MANAGER_LEVEL_SUSPENDED`.
* If there is no active connection, the limitation on power level is removed.

For more information about the CAF power levels, see the :ref:`caf_power_manager` documentation page.
