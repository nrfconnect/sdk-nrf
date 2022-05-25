.. _app_event_manager_profiler_tracer:

Application Event Manager profiler tracer
#########################################

.. contents::
   :local:
   :depth: 2

The Application Event Manager profiler tracer is a library that enables profiling Application Event Manager events.
It serves as a linking layer between :ref:`app_event_manager` and the :ref:`nrf_profiler` and allows to profile Application Event Manager event submissions and processings as the Profiler events.
The Profiler allows you to observe the propagation of an nrf_profiler event in the system, view the data connected with the event, or create statistics.

The events managed by the Application Event Manager are structured data types that are defined by the application and can contain additional data.
The Application Event Manager profiler tracer registers :c:struct:`nrf_profiler_info` as trace_data of event types.
The tracer uses this additional information to track the usage of events within the application.

See the :ref:`app_event_manager_profiling_tracer_sample` sample for an example of how to use the library with the :ref:`nrf_profiler`.

.. _app_event_manager_profiler_tracer_config:

Configuration
*************

To use the Application Event Manager profiler tracer, enable the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER` Kconfig option.
This Kconfig option also automatically initializes the Profiler.

Additional configuration
========================

You can also set the following Kconfig options when working with the Application Event Manager profiler tracer:

* :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION` - With this Kconfig option set, the Application Event Manager profiler tracer will track two additional events that mark the start and the end of each event execution, respectively.
* :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_PROFILE_EVENT_DATA` - With this Kconfig option set, the Application Event Manager profiler tracer will trigger logging of event data during profiling, allowing you to see what event data values were sent.

.. _app_event_manager_profiler_tracer_em_implementation:

Implementing profiling for Application Event Manager events
***********************************************************

.. note::
	Before you complete the following steps, make sure to :ref:`implement Application Event Manager events and modules <app_event_manager_implementing_events>`.

To profile an Application Event Manager event, you must complete the following steps:

1. Enable profiling Application Event Manager events using the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER` Kconfig option.
#. Edit the source file for the event type:

   a. Define a profiling function that logs the event data to a given buffer by calling one of the following functions for every registered data type:

      * :c:func:`nrf_profiler_log_encode_uint32`
      * :c:func:`nrf_profiler_log_encode_int32`
      * :c:func:`nrf_profiler_log_encode_uint16`
      * :c:func:`nrf_profiler_log_encode_int16`
      * :c:func:`nrf_profiler_log_encode_uint8`
      * :c:func:`nrf_profiler_log_encode_int8`
      * :c:func:`nrf_profiler_log_encode_string`

      The :c:func:`nrf_profiler_log_start` and :c:func:`nrf_profiler_log_send` are called automatically by the Application Event Manager profiler tracer.
      You do not need to call these functions for Application Event Manager events.
      Mapping the nRF Profiler event ID to an Application Event Manager event is also handled by the Application Event Manager profiler tracer.
   #. Define an :c:struct:`nrf_profiler_info` structure using :c:macro:`APP_EVENT_INFO_DEFINE` in your event source file and provide it as an argument when defining the event type with :c:macro:`APP_EVENT_TYPE_DEFINE` macro.
      This structure contains a profiling function and information about the data fields that are logged.
      The following code example shows a profiling function for the event type ``sample_event``:

      .. code-block:: c

         static void profile_sample_event(struct log_event_buf *buf, const struct app_event_header *aeh)
		{
			struct sample_event *event = cast_sample_event(aeh);

			nrf_profiler_log_encode_int8(buf, event->value1);
			nrf_profiler_log_encode_int16(buf, event->value2);
			nrf_profiler_log_encode_int32(buf, event->value3);
		}

      The following code example shows how to define the event profiling information structure and add it to event type definition:

      .. code-block:: c

         APP_EVENT_INFO_DEFINE(sample_event,
				/* Profiled datafield types. */
				ENCODE(NRF_PROFILER_ARG_S8, NRF_PROFILER_ARG_S16, NRF_PROFILER_ARG_S32),
				/* Profiled data field names - displayed by nRF Profiler. */
				ENCODE("value1", "value2", "value3"),
				/* Function used to profile event data. */
				profile_sample_event);

         APP_EVENT_TYPE_DEFINE(sample_event,
				log_sample_event,	/* Function for logging event data. */
				&sample_event_info,	/* Structure with data for profiling. */
				APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));	/* Flags managing event type. */

      .. note::
         * By default, all Application Event Manager events that are defined with an :c:struct:`event_info` argument are profiled.
         * :c:struct:`sample_event_info` is defined within the :c:macro:`APP_EVENT_INFO_DEFINE` macro.

#. Use the Profiler Python scripts to profile the application.
   See :ref:`nrf_profiler_backends` in the Profiler documentation for details.

Implementation details
**********************

The profiler tracer uses some of the Application Event Manager hooks to connect with the manager.

Initialization hook usage
=========================

.. include:: app_event_manager.rst
   :start-after: em_initialization_hook_start
   :end-before: em_initialization_hook_end

The Application Event Manager profiled tracer uses the hook to append itself to the initialization procedure.

Tracing hook usage
==================

.. include:: app_event_manager.rst
   :start-after: em_initialization_hook_start
   :end-before: em_initialization_hook_end

The Application Event Manager profiled tracer uses the hook to append itself to the initialization procedure.

.. include:: app_event_manager.rst
   :start-after: em_tracing_hooks_start
   :end-before: em_tracing_hooks_end

The Application Event Manager profiler tracer uses the tracing hooks to register nRF Profiler events and log their occurrence when application is running.

API documentation
*****************

| Header file: :file:`include/app_event_manager_profiler_tracer.h`
| Source files: :file:`subsys/app_event_manager_profiler_tracer/`

.. doxygengroup:: app_event_manager_profiler_tracer
   :project: nrf
   :members:
