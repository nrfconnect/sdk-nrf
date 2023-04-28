.. _bas_client_readme:

GATT Battery Service (BAS) Client
#################################

.. contents::
   :local:
   :depth: 2

The Battery Service Client can be used to retrieve information about the battery level from a device that provides a Battery Service Server.

The client supports a simple `Battery Service <Battery Service Specification_>`_ with one characteristic.
The Battery Level Characteristic holds the battery level in percentage units.


The Battery Service Client is used in the :ref:`central_bas` sample.

Usage
*****

.. note::
   Do not access any of the values in the :c:struct:`bt_bas_client` object structure directly.
   All values that should be accessed have accessor functions.
   The reason that the structure is fully defined is to allow the application to allocate the memory for it.

There are different ways to retrieve the battery level:

Notifications
  Use the :c:func:`bt_bas_subscribe_battery_level` function to receive notifications from the connected Battery Service.
  The notifications are passed to the provided callback function.

  The Battery Level Characteristic does not need to support notifications.
  If the server does not support notifications, read the current value as described below instead.

Reading the current value
  Use the :c:func:`bt_bas_read_battery_level` function to read the battery level.
  When subscribing to notifications, you can call this function to retrieve the current value even when there is no change.

Periodically reading the current value with time interval
  Use the :c:func:`bt_bas_start_per_read_battery_level` function to periodically read the battery level with a given time interval.
  You can call this function only when notification support is not enabled in BAS.
  See :ref:`bas_client_readme_periodic` for more details.

Getting the last known value
  The BAS Client stores the last known battery level information internally.
  Use the :c:func:`bt_bas_get_last_battery_level` function to access it.

  .. note::
     The internally stored value is updated every time a notification or read response is received.
     If no value is received from the server, the get function returns :c:macro:`BT_BAS_VAL_INVALID`.

.. _bas_client_readme_periodic:

Periodic reading of the battery level
*************************************

You can use the :c:func:`bt_bas_start_per_read_battery_level` function to periodically read the battery level with a specific time interval.
This function sends a read request to the connected device periodically.
It can be used only when support for notifications is not enabled in BAS.

For many devices, the battery level value does not change frequently.
Depending on the type of connected device, you can decide how often to read the battery level value.
For example, if the expected battery life is in the order of years, reading the battery level value more frequently than once a week is not recommended.

You can change the periodic read interval while the periodic read is active.
In such case, the next read period is started with the new interval.

.. note::
   Providing the ``K_NO_WAIT`` and ``K_FOREVER`` arguments as the time interval causes reading of the characteristic value as soon and as often as possible.


API documentation
*****************

| Header file: :file:`include/bas_client.h`
| Source file: :file:`subsys/bluetooth/services/bas_client.c`

.. doxygengroup:: bt_bas_client_api
   :project: nrf
   :members:
