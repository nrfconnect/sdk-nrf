.. _hogp_readme:

GATT Human Interface Device (HID) Service Client
################################################

.. contents::
   :local:
   :depth: 2

The HID Service Client module can be used to interact with a connected HID server.
The server can, for example, be an instance of the :ref:`hids_readme`.

The HID Service Client is used in the :ref:`bluetooth_central_hids` sample.


The client uses the :ref:`gatt_dm_readme` module to acquire all attribute handles that are required to interact with the HID server.
Some additional data must be read from the discovered descriptors before the HID client is ready.
This process is started automatically just after the handles are assigned.
If the process finishes successfully, the :c:type:`bt_hogp_ready_cb` function is called.
Otherwise, :c:type:`bt_hogp_prep_fail_cb` is called.


Configuration
*************

Apart from standard configuration parameters, there is one important setting:

:option:`CONFIG_BT_HOGP_REPORTS_MAX`
  Sets the maximum number of total reports supported by the library.
  The report memory is shared along all HIDS client objects, so this option should be set to the maximum total number of reports supported by the application.

Usage
*****

.. note::
   Do not access any of the values in the :c:struct:`bt_hogp` object structure directly.
   All values that should be accessed have accessor functions.
   The reason that the structure is fully defined is to allow the application to allocate the memory for it.


Retrieving the HIDS client readiness state
==========================================

The following functions are available to retrieve the readiness state of the service client object:

:c:func:`bt_hogp_assign_check`
  Checks if :c:func:`bt_hogp_handles_assign` was already called and the post discovery process has been started.

:c:func:`bt_hogp_ready_check`
  Checks if the client object is fully ready to interact with the HID server.
  The ready flag is set just before the :c:type:`bt_hogp_ready_cb` function is called.


Reading the report map
======================

To read the report map, call :c:func:`bt_hogp_map_read`.
If the report map does not fit into a single PDU, call the function repeatedly with different offsets.

There is no specific support for HID report map interpretation implemented in the HIDS client.


Accessing the reports
=====================

To read or write a report, use one of the following functions:

* :c:func:`bt_hogp_rep_read`
* :c:func:`bt_hogp_rep_write`
* :c:func:`bt_hogp_rep_write_wo_rsp`

To manage input report notifications, use the following functions:

* :c:func:`bt_hogp_rep_subscribe`
* :c:func:`bt_hogp_rep_unsubscribe`

The report size is always updated before the callback function is called while reading or notifying.
It can be obtained by calling :c:func:`bt_hogp_rep_size`.

All report operations require a report info pointer as input.
How to retrieve this pointer depends on if you are processing a normal report or a boot report.


Normal report:
   The report info pointer for a normal report can be retrieved with the :c:func:`bt_hogp_rep_next` function.
   This function iterates through all detected reports (excluding boot reports).
   To find a specific report, use :c:func:`bt_hogp_rep_find`.
   This function locates a report based on its type and ID.

Boot report:
   If the connected device supports the boot protocol, it must have mouse and/or keyboard boot reports available.
   This means that:

   * For the mouse boot protocol, the function :c:func:`bt_hogp_rep_boot_mouse_in` returns a non-NULL value.
   * For the keyboard boot protocol, the two functions :c:func:`bt_hogp_rep_boot_kbd_in` and :c:func:`bt_hogp_rep_boot_kbd_out` return a non-NULL value.

   All these functions return report pointers that may be used in the access functions.
   Note, however, that these pointers cannot be used as a previous record pointer in :c:func:`bt_hogp_rep_next`.


Switching between boot and report mode
======================================

To switch between Boot Protocol Mode and Report Protocol Mode, use :c:func:`bt_hogp_pm_write`.

You can retrieve the current protocol with :c:func:`bt_hogp_pm_get`.
This function returns the internally cached version of the current protocol mode.
To update this value directly from the device, use :c:func:`bt_hogp_pm_update`.

.. note::
   Every time the protocol mode is changed, the :c:type:`bt_hogp_pm_update_cb` function is called.


Suspending and resuming
=======================

To suspend the connected device, call :c:func:`bt_hogp_suspend`.

To resume, call :c:func:`bt_hogp_exit_suspend`.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/hogp.h`
| Source file: :file:`subsys/bluetooth/services/hogp.c`

.. doxygengroup:: bt_hogp
   :project: nrf
   :members:
