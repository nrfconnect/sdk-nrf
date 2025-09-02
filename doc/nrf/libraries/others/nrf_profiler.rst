.. _nrf_profiler:

nRF Profiler
############

.. contents::
   :local:
   :depth: 2

The nRF Profiler library allows for performance measurements of an embedded device.
The library provides an interface for lightweight logging of nRF Profiler events together with associated timestamp and data.
This allows you to deliver information about the system state with minimal negative impact on performance.
You can use the module to profile :ref:`app_event_manager` events or custom events.

The nRF Profiler supports one backend that provides output to the host computer using RTT.
You can use a dedicated set of host tools available in the |NCS| to visualize and analyze the collected nRF Profiler events.
See the :ref:`nrf_profiler_script` page for details.

See the :ref:`nrf_profiler_sample` sample for an example of how to use the nRF Profiler.

.. _nrf_profiler_configuration:

Configuration
*************

Since Application Event Manager events are converted to nRF Profiler events by the :ref:`app_event_manager_profiler_tracer`, you can configure the nRF Profiler to profile custom events or Application Event Manager events, or both.

Configuring for use with custom events
======================================

To use the nRF Profiler for custom events, complete the following steps:

1. Enable the :kconfig:option:`CONFIG_NRF_PROFILER` Kconfig option.
   This option adds the nRF Profiler source code to the application.
#. Call :c:func:`nrf_profiler_init` during the application start to initialize the nRF Profiler.
#. Profile custom events, as described in the following section.

.. _nrf_profiler_profiling_custom_events:

Profiling custom events
-----------------------

To profile custom events, complete the following steps:

1. Register the custom events using :c:func:`nrf_profiler_register_event_type`.
   The following code example shows how to register event types:

   .. code-block:: c

      static const char * const data_names[] = {"value1", "value2", "value3", "value4", "string"};
      static const enum nrf_profiler_arg data_types[] = {NRF_PROFILER_ARG_U32, NRF_PROFILER_ARG_S32,
                  NRF_PROFILER_ARG_S16, NRF_PROFILER_ARG_U8,
                  NRF_PROFILER_ARG_STRING};

      no_data_event_id = nrf_profiler_register_event_type("no_data_event", NULL,
                  NULL, 0);
      data_event_id = nrf_profiler_register_event_type("data_event", data_names,
                  data_types, 5);

#. Add a structure for sending information about event occurrences:

   a. Add the following mandatory functions to the structure:

      * :c:func:`nrf_profiler_log_start` - Start logging.
      * :c:func:`nrf_profiler_log_send` - Send profiled data.

   #. Add one or more of the following optional functions in-between the mandatory functions, depending on the data format:

      * :c:func:`nrf_profiler_log_encode_uint32` - Add 32-bit unsigned integer connected with the event.
      * :c:func:`nrf_profiler_log_encode_int32` - Add 32-bit integer connected with the event.
      * :c:func:`nrf_profiler_log_encode_uint16` - Add 16-bit unsigned integer connected with the event.
      * :c:func:`nrf_profiler_log_encode_int16` - Add 16-bit integer connected with the event.
      * :c:func:`nrf_profiler_log_encode_uint8` - Add 8-bit unsigned integer connected with the event.
      * :c:func:`nrf_profiler_log_encode_int8` - Add 8-bit integer connected with the event.
      * :c:func:`nrf_profiler_log_encode_string` - Add string connected with the event.

#. Wrap the calls in one function that you then call to profile event occurrences.
   The following code example shows a function for profiling an event with data:

   .. code-block:: c

      static void profile_data_event(uint32_t val1, int32_t val2, int16_t val3,
                  uint8_t val4, const char *string)
      {
        struct log_event_buf buf;

        nrf_profiler_log_start(&buf);
        /* Profiling data connected with an event */
        nrf_profiler_log_encode_uint32(&buf, val1);
        nrf_profiler_log_encode_int32(&buf, val2);
        nrf_profiler_log_encode_int16(&buf, val3);
        nrf_profiler_log_encode_uint8(&buf, val4);
        nrf_profiler_log_encode_string(&buf, string);
        nrf_profiler_log_send(&buf, data_event_id);
      }

.. note::
   The ``data_event_id`` and the data that is profiled with the event must be consistent with the registered event type.
   The data for every data field must be provided in the correct order.

Configuration for use with Application Event Manager
====================================================

To use the nRF Profiler for Application Event Manager events, refer to the :ref:`app_event_manager_profiler_tracer` documentation.
The Application Event Manager profiler tracer automatically initializes the nRF Profiler and then acts as a linking layer between :ref:`app_event_manager` and the nRF Profiler.

Shell integration
*****************

The nRF Profiler is integrated with Zephyr's :ref:`zephyr:shell_api` module.
You can use the :kconfig:option:`CONFIG_NRF_PROFILER_SHELL` option to add an additional subcommand set (:command:`nrf_profiler`) to the shell.
The option is enabled by default.

This subcommand set contains the following commands:

:command:`list`
  Show a list of profiled event types.
  The letters "E" or "D" indicate if profiling is currently enabled or disabled for a given event type.

:command:`enable` or :command:`disable`
  Enable or disable profiling.
  If called without additional arguments, the command applies to all event types.
  To enable or disable profiling for specific event types, pass the event type indexes (as displayed by :command:`list`) as arguments.

API documentation
*****************

| Header file: :file:`include/nrf_profiler.h`
| Source files: :file:`subsys/nrf_profiler/`

.. doxygengroup:: nrf_profiler
