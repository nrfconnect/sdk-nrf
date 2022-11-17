.. _asset_tracker_v2_app_module:

Application module
##################

.. contents::
   :local:
   :depth: 2

The application module controls key mechanisms in the Asset Tracker v2.
It decides when to sample data, what types of data to sample, and has explicit control over some aspects of the application.

Features
********

This section documents the features implemented by the module.

Sample requests
===============

Depending on the application's :ref:`Real-time configurations <real_time_configs>`, the application module sends periodic sample requests to other modules in the system.
The sample requests are of type :c:enum:`APP_EVT_DATA_GET` and contains a list of :ref:`Data types <app_data_types>` that must be sampled by the corresponding module.
Each module handles the sample requests individually and if a module finds its corresponding data type in the list, it starts sampling for that particular data type.
For instance, if the :ref:`asset_tracker_v2_location_module` finds the :c:enum:`APP_DATA_LOCATION` data type, it starts searching for a location.
Some modules, such as the :ref:`asset_tracker_v2_modem_module`, even support multiple data types.
It supports :c:enum:`APP_DATA_MODEM_STATIC`, :c:enum:`APP_DATA_MODEM_DYNAMIC`, and :c:enum:`APP_DATA_BATTERY`.
In addition to the sample list, the module also includes a sample timeout in the :c:enum:`APP_EVT_DATA_GET` event.
This timeout sets how much time each module has to sample data before the :ref:`asset_tracker_v2_data_module` sends what is available.

.. note::
   Sample requests are sent either periodically or upon detection of movement.
   The timeouts and modes that have an impact on the frequency of sample requests are
   documented in :ref:`Real-time configurations <real_time_configs>`.

Application start
=================

The application module is the system's initial point of entry and the source file of the module is :file:`asset_tracker_v2/src/main.c`.
When the application boots, the module initializes the :ref:`app_event_manager` and sends out the initial event :c:enum:`APP_EVT_START` that starts the rest of the modules in the application.
It also initializes the :ref:`caf_overview` by calling the :c:func:`module_set_state` API with the :c:enum:`MODULE_STATE_READY` state.

Configuration options
*********************

There are no configuration options for the application module.

Module states
*************

The application module has an internal state machine with the following states:

* ``STATE_INIT`` - The initial state of the module.
* ``STATE_RUNNING`` - The module has received its initial configuration from the data module and has started the appropriate timers.

   * ``SUB_STATE_ACTIVE_MODE`` - The application is in the active mode. In this state, the module sends out sample requests periodically.
   * ``SUB_STATE_PASSIVE_MODE`` - The application is in the passive mode. In this state, the module sends out a sample request upon movement.

* ``STATE_SHUTDOWN`` - The module has been shut down after receiving a request from the utility module.

Module events
*************

The :file:`asset_tracker_v2/src/events/app_module_event.h` header file contains a list of events sent by the module.

Dependencies
************

This module uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`
* :ref:`nrf_modem_lib_readme`
* :ref:`caf_overview`

API documentation
*****************

| Header file: :file:`asset_tracker_v2/src/events/app_module_event.h`
| Source files: :file:`asset_tracker_v2/src/events/app_module_event.c`, :file:`asset_tracker_v2/src/main.c`

.. doxygengroup:: app_module_event
   :project: nrf
   :members:
