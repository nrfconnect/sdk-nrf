.. _api_modules_common:

Modules common API
##################

.. contents::
   :local:
   :depth: 2

The Modules common API provides the functionality to administer mechanisms in the application architecture, and consist of functions that are shared between modules.
The API supports the following functionalities:

* Registering and starting a module using :c:func:`module_start`.
* Deregistering a module using :c:func:`modules_shutdown_register`.
* Enqueueing and dequeueing message queue items using :c:func:`module_get_next_msg` and :c:func:`module_enqueue_msg`.
* Macros used to handle :ref:`Application Event Manager <app_event_manager>` events sent between modules.

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/modules/modules_common.h`
| Source files: :file:`asset_tracker_v2/src/modules_common.c`

.. doxygengroup:: modules_common
   :project: nrf
   :members:
