.. _nrf_desktop_bas:

GATT Battery Service module
###########################

.. contents::
   :local:
   :depth: 2

Use the GATT Battery Service module to enable and define the GATT Battery Service, and notify the subscribers about the battery level changes.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_bas_start
    :end-before: table_bas_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To enable this module, use the :option:`CONFIG_DESKTOP_BAS_ENABLE` Kconfig option, that is implied by the :option:`CONFIG_DESKTOP_BT_PERIPHERAL` option.
The Battery Service is required for the HID peripheral device.
For more information about the Bluetooth® configuration in the nRF Desktop, see the :ref:`nrf_desktop_bluetooth_guide` documentation.

Implementation details
**********************

The module uses :c:macro:`BT_GATT_SERVICE_DEFINE` to define the GATT Service.
On ``battery_level_event``, the module updates the battery level and notifies the subscribers.

More detailed information about GATT are available in the Zephyr documentation: :ref:`zephyr:bt_gatt`.
