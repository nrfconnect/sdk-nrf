.. _gatt_dm_readme:

GATT Discovery Manager
######################

.. contents::
   :local:
   :depth: 2

The GATT Discovery Manager handles service discovery on GATT servers.

When a client connects to a peer device that has a desired server, service discovery is necessary to ensure that the client interacts with the server's characteristics using the correct attributes handles.
Service discovery is also important because Bluetooth® LE advertising does not mandate that all services are advertised.
To actually know if a service is present on a peer device, you must perform a service discovery.

The GATT Discovery Manager simplifies the usage of :ref:`Zephyr <bluetooth_api>`'s :c:func:`bt_gatt_discover` function by processing the data using predefined filters.

The GATT Discovery Manager is used, for example, in the :ref:`bluetooth_central_hids` sample.

Limitations
***********

Only one discovery procedure at a time can be running.

API documentation
*****************

| Header file: :file:`include/bluetooth/gatt_dm.h`
| Source file: :file:`subsys/bluetooth/gatt_dm.c`

.. doxygengroup:: bt_gatt_dm
   :project: nrf
   :members:
