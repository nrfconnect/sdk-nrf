.. _event_manager_profiler_tracer:

Event Manager profiler tracer
#############################

.. contents::
   :local:
   :depth: 2

The Event Manager profiler tracer is a library that enables profiling Event Manager events.
It serves as a linking layer between :ref:`event_manager` and the :ref:`profiler` and allows to profile Event Manager event submissions and processings as the Profiler events.
The Profiler allows you to observe the propagation of an profiler event in the system, view the data connected with the event, or create statistics.

The events managed by the Event Manager are structured data types that are defined by the application and can contain additional data.
The Event Manager profiler tracer registers :c:struct:`profiler_info` as trace_data of event types.
The tracer uses this additional information to track the usage of events within the application.

See the :ref:`event_manager_profiling_sample` sample for an example of how to use the library with the :ref:`profiler`.

.. _event_manager_profiler_tracer_config:

Configuration
*************

To use the Event Manager profiler tracer, enable the :kconfig:`CONFIG_EVENT_MANAGER_PROFILER` Kconfig option.
This Kconfig option also automatically initializes the Profiler.

Additional configuration
========================

You can also set the following Kconfig options when working with the Event Manager profiler tracer:

* :kconfig:`CONFIG_EVENT_MANAGER_PROFILER_TRACE_EVENT_EXECUTION` - With this Kconfig option set, the Event Manager profiler tracer will track two additional events that mark the start and the end of each event execution, respectively.
* :kconfig:`CONFIG_EVENT_MANAGER_PROFILER_PROFILE_EVENT_DATA` - With this Kconfig option set, the Event Manager profiler tracer will trigger logging of event data during profiling, allowing you to see what event data values were sent.

.. _event_manager_profiler_tracer_em_implementation:

Implementing profiling for Event Manager events
***********************************************

.. note::
	Before you complete the following steps, make sure to :ref:`implement Event Manager events and modules <event_manager_implementing_events>`.

To profile an Event Manager event, you must complete the following steps:

1. Enable profiling Event Manager events using the :kconfig:`CONFIG_EVENT_MANAGER_PROFILER` Kconfig option.
#. Edit the source file for the event type:

   a. Define a profiling function that logs the event data to a given buffer by calling one of the following functions for every registered data type:

      * :c:func:`profiler_log_encode_uint32`
      * :c:func:`profiler_log_encode_int32`
      * :c:func:`profiler_log_encode_uint16`
      * :c:func:`profiler_log_encode_int16`
      * :c:func:`profiler_log_encode_uint8`
      * :c:func:`profiler_log_encode_int8`
      * :c:func:`profiler_log_encode_string`

      The :c:func:`profiler_log_start` and :c:func:`profiler_log_send` are called automatically by the Event Manager profiler tracer.
      You do not need to call these functions for Event Manager events.
      Mapping the profiler event ID to an Event Manager event is also handled by the Event Manager profiler tracer.
   #. Define an :c:struct:`profiler_info` structure using :c:macro:`EVENT_INFO_DEFINE` in your event source file and provide it as an argument when defining the event type with :c:macro:`EVENT_TYPE_DEFINE` macro.
      This structure contains a profiling function and information about the data fields that are logged.
      The following code example shows a profiling function for the event type ``sample_event``:

      .. code-block:: c

         static void profile_sample_event(struct log_event_buf *buf, const struct event_header *eh)
		{
			struct sample_event *event = cast_sample_event(eh);

			profiler_log_encode_int8(buf, event->value1);
			profiler_log_encode_int16(buf, event->value2);
			profiler_log_encode_int32(buf, event->value3);
		}

      The following code example shows how to define the event profiling information structure and add it to event type definition:

      .. code-block:: c

         EVENT_INFO_DEFINE(sample_event,
				/* Profiled datafield types. */
				ENCODE(PROFILER_ARG_S8, PROFILER_ARG_S16, PROFILER_ARG_S32),
				/* Profiled data field names - displayed by profiler. */
				ENCODE("value1", "value2", "value3"),
				/* Function used to profile event data. */
				profile_sample_event);

         EVENT_TYPE_DEFINE(sample_event,
				true,
				log_sample_event,	/* Function for logging event data. */
				&sample_event_info);	/* Structure with data for profiling. */

      .. note::
         * By default, all Event Manager events that are defined with an :c:struct:`event_info` argument are profiled.
         * :c:struct:`sample_event_info` is defined within the :c:macro:`EVENT_INFO_DEFINE` macro.

#. Use the Profiler Python scripts to profile the application.
   See :ref:`profiler_backends` in the Profiler documentation for details.

Event Manager profiler tracer implementation details
****************************************************

.. include:: event_manager.rst
   :start-after: em_tracing_hooks_start
   :end-before: em_tracing_hooks_end

The Event Manager profiler tracer uses the hooks to register profiler events and log their occurrence when application is running.

API documentation
*****************

| Header file: :file:`include/event_manager_profiler.h`
| Source files: :file:`subsys/event_manager_profiler/`

.. doxygengroup:: event_manager_profiler
   :project: nrf
   :members:
