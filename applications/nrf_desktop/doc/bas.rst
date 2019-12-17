.. _bas:

GATT Battery Service
####################

Use the ``bas`` module to enable and define the GATT Battery Service, and notify the subscribers about the battery level changes.

Module Events
*************

+---------------------+-------------------------+-------------+---------------+------------------+
| Source Module       | Input Event             | This Module | Output Event  | Sink Module      |
+=====================+=========================+=============+===============+==================+
| :ref:`battery_meas` | ``battery_level_event`` | ``bas``     |               |                  |
+---------------------+-------------------------+-------------+---------------+------------------+

Configuration
*************

The module is enabled with the ``CONFIG_DESKTOP_BAS_ENABLE`` option.
The option is selected by ``CONFIG_DESKTOP_HID_PERIPHERAL`` -- Battery Service is required for the HID peripheral device.

Implementation details
**********************

The module uses :c:macro:`BT_GATT_SERVICE_DEFINE` to define the GATT Service.
On ``battery_level_event``, the module updates the battery level and notifies the subscribers.

More detailed information about GATT are available in the Zephyr documentation: :ref:`zephyr:bt_gatt`.
