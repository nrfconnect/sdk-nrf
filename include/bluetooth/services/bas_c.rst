.. _bas_c_readme:

GATT Battery Service (BAS) Client
#################################

The Battery Service Client can be used to retrieve information about the battery level from a device that provides a Battery Service Server.

The client supports a simple `Battery Service <Battery Service Specification_>`_ with one characteristic.
The Battery Level Characteristic holds the battery level in percentage units.


The Battery Service Client is used in the :ref:`central_bas` sample.

Usage
*****

.. note::
   Do not access any of the values in the :cpp:type:`bt_gatt_bas_c` object structure directly.
   All values that should be accessed have accessor functions.
   The reason that the structure is fully defined is to allow the application to allocate the memory for it.

There are different ways to retrieve the battery level:

Notifications
  Use :cpp:func:`bt_gatt_bas_c_subscribe` to receive notifications from the connected Battery Service.
  The notifications are passed to the provided callback function.

  Note that it is not mandatory for the Battery Level Characteristic to support notifications.
  If the server does not support notifications, read the current value as described below instead.

Reading the current value
  Use :cpp:func:`bt_gatt_bas_c_read` to read the battery level.
  When subscribing to notifications, you can call this function to retrieve the current value even when there is no change.

Getting the last known value
  The BAS Client stores the last known battery level information internally.
  Use :cpp:func:`bt_gatt_bas_c_get` to access it.

  .. note::
     The internally stored value is updated every time a notification or read response is received.
     If no value is received from the server, the get function returns :cpp:type:`BT_GATT_BAS_VAL_INVALID`.


API documentation
*****************

| Header file: :file:`include/bas_c.h`
| Source file: :file:`subsys/bluetooth/services/bas_c.c`

.. doxygengroup:: bt_gatt_bas_c
   :project: nrf
   :members:
