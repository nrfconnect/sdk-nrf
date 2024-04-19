.. _hogp_readme:

GATT Human Interface Device Service (HIDS) Client
#################################################

.. contents::
   :local:
   :depth: 2

The HIDS Client uses the :ref:`gatt_dm_readme` to acquire all attribute handles that are required to interact with the HIDS server.

Overview
********

Some additional data must be read from the discovered descriptors before the HIDS Client is ready.
This process is started automatically just after the handles are assigned.
If the process finishes successfully, the :c:type:`bt_hogp_ready_cb` function is called.
Otherwise, :c:type:`bt_hogp_prep_fail_cb` is called.

Configuration
*************

Use the following Kconfig options to configure the library:

* :kconfig:option:`CONFIG_BT_HOGP` - Enable the library in the application build.
* :kconfig:option:`CONFIG_BT_HOGP_REPORTS_MAX` - Set the maximum number of total reports supported by the library.

  The report memory is shared with all HIDS Client objects, so set this option to the maximum total number of reports supported by the application.

Usage
*****

You can use the GATT HIDS Client library to interact with a connected HIDS server.

Retrieving the HIDS Client readiness state
==========================================

The following functions are available to retrieve the readiness state of the service client object:

* :c:func:`bt_hogp_assign_check` - Check if :c:func:`bt_hogp_handles_assign` was called and the post-discovery process started.
* :c:func:`bt_hogp_ready_check` - Check if the client object is ready to interact with the HIDS server.
  The ready flag is set just before the :c:type:`bt_hogp_ready_cb` function is called.

Reading the report map
======================

To read the report map, call the :c:func:`bt_hogp_map_read` function.
If the report map does not fit into a single PDU, call the function repeatedly with different offsets.

The HIDS report map interpretation implemented in the HIDS Client is not specifically supported.

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
How to retrieve this pointer depends on whether you are processing a normal report or a boot report.

.. tabs::

   .. group-tab:: Normal report

      Call the :c:func:`bt_hogp_rep_next` function to retrieve the report info pointer.
      This function iterates through all detected reports (excluding boot reports).

      Use the :c:func:`bt_hogp_rep_find` function to find a specific report.
      This function locates a report based on its type and ID.

   .. group-tab:: Boot report

      If the connected device supports the boot protocol, it must have mouse or keyboard boot reports available.
      This means that:

      * For the mouse boot protocol, the function :c:func:`bt_hogp_rep_boot_mouse_in` returns a non-NULL value.
      * For the keyboard boot protocol, the two functions :c:func:`bt_hogp_rep_boot_kbd_in` and :c:func:`bt_hogp_rep_boot_kbd_out` return a non-NULL value.

      All these functions return report pointers that may be used in the access functions.
      However, these pointers cannot be used as a previous record pointer in :c:func:`bt_hogp_rep_next`.

Changing protocol mode
======================

To manage protocol modes, use the following functions:

* :c:func:`bt_hogp_pm_write` - Switch between Boot Protocol Mode and Report Protocol Mode.

  Every time the protocol mode is changed, the :c:type:`bt_hogp_pm_update_cb` function is called.

* :c:func:`bt_hogp_pm_get` - Retrieve the current protocol.

  This function returns the internally cached version of the current protocol mode.

* :c:func:`bt_hogp_pm_update` - Update the protocol mode value directly from the device.

Suspending and resuming
=======================

To suspend or resume the connected device, call the following functions:

* :c:func:`bt_hogp_suspend` - Suspend the connected device.
* :c:func:`bt_hogp_exit_suspend` - Resume the connected device.

Samples using the library
*************************

The following |NCS| modules use this library:

* :ref:`hids_readme`
* :ref:`nrf_desktop_hid_forward`

The following |NCS| application uses this library:

* :ref:`nrf_desktop`

Additional information
**********************

Do not access any of the values in the :c:struct:`bt_hogp` object structure directly.
All values that should be accessed have accessor functions.
The structure is fully defined to allow the application to allocate the memory for it.

Dependencies
************

There are no dependencies for using this library.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/hogp.h`
| Source file: :file:`subsys/bluetooth/services/hogp.c`

.. doxygengroup:: bt_hogp
   :project: nrf
   :members:
