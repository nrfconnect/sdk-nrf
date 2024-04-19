.. _lib_nrf_bt_scan_readme:
.. _nrf_bt_scan_readme:

Bluetooth LE scanning
#####################

.. contents::
   :local:
   :depth: 2

This library handles the Bluetooth® Low Energy scanning for your application.

Overview
********

Use the library to find advertising devices and establish connections with them.
It can automatically establish connection after a filter match.
It can also receive :ref:`directed advertising packets <lib_nrf_bt_scan_readme_directedadvertising>`.

The library can work in one of the following modes:

* Simple mode - Works without filters.
* Advanced mode - Allows using advanced filters.

Configuration
*************

Use the :kconfig:option:`CONFIG_BT_SCAN` Kconfig option to enable the library in the build system.

Blocklist
=========

Devices can be added to the blocklist, which means that the library ignores these devices and does not generate any events for them.
Use the :kconfig:option:`CONFIG_BT_SCAN_BLOCKLIST` Kconfig option to enable the blocklist.

In the default configuration, the library allows to add up to two devices to the blocklist.
To increase the blocklist size, set the :kconfig:option:`CONFIG_BT_SCAN_BLOCKLIST_LEN` Kconfig option.
Use the :c:func:`bt_scan_blocklist_device_add` function to add a new device to the blocklist.
To remove all devices from the blocklist, use the :c:func:`bt_scan_blocklist_clear` function.

.. _lib_nrf_bt_scan_readme_directedadvertising:

Directed advertising
====================

To receive directed advertising packets through the library, enable one of the following Kconfig options in Zephyr:

* :kconfig:option:`CONFIG_BT_PRIVACY` - Scan with changing addresses.
* :kconfig:option:`CONFIG_BT_SCAN_WITH_IDENTITY` - Scan with a local identity address.

It is recommended to enable the :kconfig:option:`CONFIG_BT_PRIVACY` Kconfig option to support directed advertising only between bonded peers.
Use the :kconfig:option:`CONFIG_BT_SCAN_WITH_IDENTITY` Kconfig option only when the :kconfig:option:`CONFIG_BT_PRIVACY` Kconfig option is not available.

When the library is set to the advanced mode and :ref:`filters <lib_nrf_bt_scan_readme_filters>` are set, you can use the ``filter_no_match`` event to check if directed advertising packets have been received.
They will typically not match any filter as, by specification, they do not contain any advertising data.

If there is no match, you can establish a connection without the need to disable or reconfigure the existing filters.

The following code sample demonstrates the usage of the ``filter_no_match`` event:

.. literalinclude:: ../../../../samples/bluetooth/central_hids/src/main.c
    :language: c
    :start-after: include_startingpoint_scan_rst
    :end-before: include_endpoint_scan_rst

Usage
*****

You can use the library to execute actions described in the following sections.

Initializing
============

To initialize the library, call the :c:func:`bt_scan_init` function.

You can also call the function without an initialization structure.
When you pass the initialization structure as ``NULL``, the default static configuration is used.

This configuration is also used when you initialize the library with a structure that has ``NULL`` pointers set for scan and connection parameters.

Scanning
========

Call the following functions to perform basic scanning activities:

* :c:func:`bt_scan_start` - Start scanning.

  In the simple mode, when you do not use the event handler, you can establish a connection when the scanner finds the device.

* :c:func:`bt_scan_params_set` - Change parameters.

* :c:func:`bt_scan_stop` - Stop scanning manually.

  Scanning stops automatically if the library established the connection.

* :c:func:`bt_scan_start` - Resume scanning.

.. _lib_nrf_bt_scan_readme_filters:

Filtering
=========

Use filters in the advanced mode to narrow down the scan to devices of a specific type and mode.

Filter types
------------

See the following table for the details on the available filter types:

+-------------+---------------------------------------------+
| Filter type | Details                                     |
+=============+=============================================+
| Name        | The filter is set to the target name.       |
+-------------+---------------------------------------------+
| Short name  | The filter is set to the target short name. |
+-------------+---------------------------------------------+
| Address     | The filter is set to the target address.    |
+-------------+---------------------------------------------+
| UUID        | The filter is set to the target UUID.       |
+-------------+---------------------------------------------+
| Appearance  | The filter is set to the target appearance. |
+-------------+---------------------------------------------+

Filter modes
------------

See the following table for the details on the behavior of the available filter modes:

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
--------------------------

After a scanning session is started by the application, some peripheral devices can disconnect immediately.
This might result in a loop of the connection and disconnection events.

To avoid that, enable the :kconfig:option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER` Kconfig option that limits the number of the connection attempts.

This filter automatically tracks the connected devices and counts all disconnection events for them.
If the number of disconnections is greater than or equal to the number of the allowed attempts, the library ignores this device.

You must remove the filtered devices manually from the filter array through the :c:func:`bt_scan_conn_attempts_filter_clear` function.
Use this function before each scan starts, unless your application has different requirements.
If the filter array is full, the library overwrites the oldest device with the new one.

In the default configuration, the filter allows to add two devices and limits the connection attempts to two.
To increase the number of devices, set the :kconfig:option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER_LEN` Kconfig option.
The :kconfig:option:`CONFIG_BT_SCAN_CONN_ATTEMPTS_COUNT` Kconfig option adjusts the number of connection attempts.

Samples using the library
*************************

The :ref:`nrf_desktop` application uses this library.

Dependencies
************

There are no dependencies for using this library.

API documentation
*****************

| Header file: :file:`include/bluetooth/scan.h`
| Source file: :file:`subsys/bluetooth/scan.c`

.. doxygengroup:: nrf_bt_scan
   :project: nrf
   :members:
