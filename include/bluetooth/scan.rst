.. _nrf_bt_scan_readme:

Scanning module
###############

The Scanning Module handles the BLE scanning for your application. You can use it to find an advertising device and establish a connection with it. The scan can be narrowed down to the device of a specific type by using filters.

The Scanning Module can work in one of the following modes:

* simple mode without using filters, or
* advanced mode that allows you to use advanced filters.

The module can automatically establish a connection after a filter match.

Usage
*****

.. list-table::
   :header-rows: 1

   * - Action
     - Description
   * - Initialization
     - The module must be initialized with :cpp:func:`bt_scan_init` that can be called without initialization structure. When initialization structure is passed as NULL, the default static configuration is used. This configuration is also used when you use an initialization structure with NULL pointers to scan parameters and connection parameters.
   * - Starting scanning
     - When initialization completes, you can start scanning with :cpp:func:`bt_scan_start`. In the simplest mode when you do not using event handler, the connection can be established when scanner found device.
   * - Changing parameters
     - Call the function :cpp:func:`bt_scan_params_set` to change parameters.
   * - Stopping scanning
     - Call the function :cpp:func:`bt_scan_stop` to stop scanning.
   * - Resuming scanning
     - Scanning stops if the module established the connection automatically, or if the application calls :cpp:func:`bt_scan_stop`. To resume scanning use :cpp:func:`bt_scan_start`.


Filters
*******

The module can set scanning filters of different type and mode.

Filters types
=============

+-------------+--------------------------------------+
| Filter tupe | Details                              |
+=============+======================================+
| Name        | Filter set to the target name.       |
+-------------+--------------------------------------+
| Short name  | Filter set to the short target name. |
+-------------+--------------------------------------+
| Address     | Filter set to the target address.    |
+-------------+--------------------------------------+
| UUID        | Filter set to the target UUID.       |
+-------------+--------------------------------------+
| Appearance  | Filter set to the target appearance. |
+-------------+--------------------------------------+


Filter modes
============

+-----------------------------------------------------------------------------------------------+
| Filter mode | Behavior                                                                        |
+=============+=================================================================================+
| Normal      | Only one of the filters you set, regardless of the type, must be matched to     |
|             | call filter match callback                                                      |
+-------------+---------------------------------------------------------------------------------+
| Multifilter | In this mode, at least one filter from each filter type you set must be         |
|             | matched to call filter match callback, with UUID as an exception: all specified |
|             | UUIDs must match in this mode.                                                  |
|             |                                                                                 |
|             | Example: Several filters are set for name, address, UUID, and appearance. To    |
|             | call filter match callback, the following types                                 |
|             | must match:                                                                     |
|             |                                                                                 |
|             | * one of the address filters,                                                   |
|             | * one of the name filters,                                                      |
|             | * one of the appearance filters,                                                |
|             | * all of the UUID filters.                                                      |
|             |                                                                                 |
|             | Otherwise, the not found callback is called.                                    |
+-------------+---------------------------------------------------------------------------------+


API documentation
*****************

| Header file: :file:`include/bluetooth/scan.h`
| Source file: :file:`subsys/bluetooth/scan.c`

.. doxygengroup:: nrf_bt_scan
   :project: nrf
   :members:
