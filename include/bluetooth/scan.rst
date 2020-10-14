.. _nrf_bt_scan_readme:

Scanning module
###############

.. contents::
   :local:
   :depth: 2

The scanning module handles the Bluetooth Low Energy scanning for your application.
You can use it to find advertising devices and establish connections with them.

Using :ref:`filters <nrf_bt_scan_readme_filters>`, you can narrow down the scan to devices of a specific type.

The Scanning Module can work in one of the following modes:

* Simple mode - without using filters
* Advanced mode - that allows using advanced filters

The module can automatically establish a connection after a filter match.
It can also receive :ref:`directed advertising packets <nrf_bt_scan_readme_directedadvertising>`.

Usage
*****


You can use the scanning module to execute the following actions:

Initialize
   To initialize the module, call the function :c:func:`bt_scan_init`.

   You can also call the function without an initialization structure.
   When you pass the initialization structure as ``NULL``, the default static configuration is used.

   This configuration is also used when you use an initialization structure with ``NULL`` pointers to scan parameters and connection parameters.

Start scanning
   When the initialization is completed, call the function :c:func:`bt_scan_start` to start scanning.

   In simple mode, when you do not use the event handler, you can establish the connection when the scanner finds the device.

Change parameters
   To change parameters, call the function :c:func:`bt_scan_params_set`.

Stop scanning
   Scanning stops if the module established the connection automatically.
   To manually stop scanning, call the function :c:func:`bt_scan_stop`.

Resume scanning
   To resume scanning, call the function :c:func:`bt_scan_start`.

.. _nrf_bt_scan_readme_filters:

Filters
*******

While using the scanning module in advanced mode, you can set select various filter types and modes.

Filter types
============

Check the following table for the details on the available filter types.

+-------------+--------------------------------------+
| Filter type | Details                              |
+=============+======================================+
| Name        | Filter set to the target name.       |
+-------------+--------------------------------------+
| Short name  | Filter set to the target short name. |
+-------------+--------------------------------------+
| Address     | Filter set to the target address.    |
+-------------+--------------------------------------+
| UUID        | Filter set to the target UUID.       |
+-------------+--------------------------------------+
| Appearance  | Filter set to the target appearance. |
+-------------+--------------------------------------+


Filter modes
============

Check the following table for the details on the behavior of the available filter modes.

+--------------+-----------------------------------------------------------------------------------------------------------+
| Filter mode  | Behavior                                                                                                  |
+==============+===========================================================================================================+
| Normal       | It triggers a filter match callback when only one of the filters set matches, regardless of the type.     |
+--------------+-----------------------------------------------------------------------------------------------------------+
| Multifilter  | It triggers a filter match callback only when both of the following conditions happen:                    |
|              |                                                                                                           |
|              | * All specified UUIDs match.                                                                              |
|              | * At least one filter for each of the filter types set matches.                                           |
|              |                                                                                                           |
|              | For example, several filters can be set for name, address, UUID, and appearance.                          |
|              | To trigger a filter match callback in this scenario, all of the following types must match:               |
|              |                                                                                                           |
|              | * At least one of the address filters                                                                     |
|              | * At least one of the name filters                                                                        |
|              | * At least one of the appearance filters                                                                  |
|              | * All of the UUID filters                                                                                 |
|              |                                                                                                           |
|              | If not all of these types match, the ``not found`` callback is triggered.                                 |
+--------------+-----------------------------------------------------------------------------------------------------------+

Connection attempts filter
==========================

After a scanning is started by the application, some peripheral devices can disconnect immediately for some reasons during an ongoing scanning session.
This might result in a loop of the connection and disconnection events.

To avoid this loop, you can enable the connection attempts filter that limits number of the connection attempts.
Use the :option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER` to enable this filter.

This filter automatically tracks the connected devices and counts all disconnection events for them.
If the disconnection count is greater than or equal to the number of allowed attempts, the scanning module ignores this device.

Filtered devices must be removed manually from the filter array using :cpp:func:`bt_scan_conn_attempts_filter_clear`.
It is recommended to use :cpp:func:`bt_scan_conn_attempts_filter_clear` before each scan starts, unless your application has different requirements.
If the filter array is full, the scanning module overwrites the oldest device with the new one.
In the default configuration, the filter allows to add two devices and limits the connection tries to two.

You can increase the device number by setting the configuration option :option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER_LEN`.
The option :option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_COUNT` is responsible for the number of connection attempts.

Blocklist
=========

Devices can be added to the blocklist, which means that the scanning module ignores these devices and does not generate any events for them.
Use the option :option:`CONFIG_BT_SCAN_BLOCKLIST` to enable the blocklist.

In the default configuration, the scanning module allows to add up to two devices to the blocklist.
You can increase the blocklist size by setting the option :option:`CONFIG_BT_SCAN_BLOCKLIST_LEN`.
Use the :cpp:func:`bt_scan_blocklist_device_add` function to add a new device to the blocklist.
To remove all devices from the blocklist, use :cpp:func:`bt_scan_blocklist_clear`.

.. _nrf_bt_scan_readme_directedadvertising:

Directed Advertising
====================


To receive directed advertising packets using the Scanning Module, enable one of the following options in Zephyr:

* :option:`CONFIG_BT_PRIVACY` - Scan with changing addresses
* :option:`CONFIG_BT_SCAN_WITH_IDENTITY` - Scan with a local identity address

It is recommended to enable the :option:`CONFIG_BT_PRIVACY` option to support directed advertising only between bonded peers.
Use the :option:`CONFIG_BT_SCAN_WITH_IDENTITY` option only when the :option:`CONFIG_BT_PRIVACY` option is not available.

When the scanning module is set in advanced mode and :ref:`filters <nrf_bt_scan_readme_filters>` are set, you can use the ``filter_no_match`` event to check if directed advertising packets have been received.
They will typically not match any filter as, by specification, they do not contain any advertising data.

If there is no match, you can establish a connection without the need to disable or reconfigure the existing filters.

The following code sample demonstrates the usage of the ``filter_no_match`` event:

.. literalinclude:: ../../samples/bluetooth/central_hids/src/main.c
    :language: c
    :start-after: include_startingpoint_scan_rst
    :end-before: include_endpoint_scan_rst


API documentation
*****************

| Header file: :file:`include/bluetooth/scan.h`
| Source file: :file:`subsys/bluetooth/scan.c`

.. doxygengroup:: nrf_bt_scan
   :project: nrf
   :members:
