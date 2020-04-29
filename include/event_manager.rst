.. _event_manager:

Event Manager
#############

The Event Manager provides the infrastructure for implementing applications in an event-driven architecture.
Instead of using direct function calls between modules, the modules can communicate and transfer data using events, which reduces the number of direct dependencies between modules.

The Event Manager processes all events and propagates them to the modules (listeners) that subscribe to an event type.
Multiple modules can subscribe to the same event type at the same time.
You can easily swap out listeners to, for example, create multiple configurations.
A typical use case for this is to support different hardware configurations with one application.

Events are distinguished by event type.
Listeners can process events differently based on their type.
You can easily define custom event types for your application.
Currently, up to 32 event types can be used in an application.

You can use the :ref:`profiler` to observe the propagation of an event in the system, view the data connected with the event, or create statistics.
A shell integration is available to display additional information and to dynamically enable or disable logging for given event types.

See the :ref:`event_manager_sample` sample for an example on how to use the Event Manager.

Configuration
*************

Apart from standard configuration parameters, there are several required settings:

:option:`CONFIG_LINKER_ORPHAN_SECTION_PLACE`
  The Event Manager uses orphan memory sections.
  Set this option to suppress warnings and errors.

:option:`CONFIG_HEAP_MEM_POOL_SIZE`
  Events are dynamically allocated using heap memory.
  Set this option to enable dynamic memory allocation and configure a heap size that is suitable for your application.

:option:`CONFIG_REBOOT`
  If an out-of-memory error occurs when allocating an event, the system should reboot.
  Set this option to enable the sys_reboot API.

Call :cpp:func:`event_manager_init` during the application start to initialize the Event Manager.

Events
******

Events are used for communication between modules.
Every event has a specified type.
It can also contain additional data.

To submit an event of a given type (for example, ``sample_event``), you must first allocate it by calling the function with the name new\_\ *event_type_name* (for example, ``new_sample_event()``).
You can then write values to the data fields.
Finally, use :c:macro:`EVENT_SUBMIT` to submit the event.

The following code example shows how to create and submit an event of type ``sample_event`` that has three data fields:

.. code-block:: c

	/* Allocate event. */
	struct sample_event *event = new_sample_event();

	/* Write data to datafields. */
	event->value1 = value1;
	event->value2 = value2;
	event->value3 = value3;

	/* Submit event. */
	EVENT_SUBMIT(event);

After the event is submitted, the Event Manager adds it into the processing queue.
When the event is processed, the Event Manager notifies all modules that subscribe to this event type.

.. warning::

	Events are dynamically allocated and must be submitted.
	If an event is not submitted, it will not be handled and the memory will not be freed.


Implementing an event type
==========================

The Event Manager provides macros to easily create and implement custom event types.
For each event type, create a header file and a source file.

.. note::
   Currently, up to 32 event types can be used in an application.

Header file
-----------

The header file must include the Event Manager header file (``#include event_manager.h``).
To define the new event type, create a structure for it that contains ``struct event_header header`` as first field and, optionally, custom data fields.
Finally, declare the event type with the :c:macro:`EVENT_TYPE_DECLARE` macro, passing the name of the created structure as argument.

The following code example shows a header file for the event type ``sample_event``:

.. code-block:: c

	#include "event_manager.h"

	struct sample_event {
		struct event_header header;

		/* Custom data fields. */
		s8_t value1;
		s16_t value2;
		s32_t value3;
	};

	EVENT_TYPE_DECLARE(sample_event);

Source file
-----------

The source file must include the header file for the new event type.
Define the event type with the :c:macro:`EVENT_TYPE_DEFINE` macro, passing the name of the event type as declared in the header and additional parameters.
For example, you can provide a function that fills a buffer with a string version of the event data (used for logging).

The following code example shows a source file for the event type ``sample_event``:

.. code-block:: c

	#include "sample_event.h"

	static int log_sample_event(const struct event_header *eh, char *buf,
				    size_t buf_len)
	{
		struct sample_event *event = cast_sample_event(eh);

		return snprintf(buf, buf_len, "val1=%d val2=%d val3=%d", event->value1,
			event->value2, event->value3);
	}

	EVENT_TYPE_DEFINE(sample_event,		/* Unique event name. */
		  	  true, 		/* Event logged by default. */
		  	  log_sample_event, 	/* Function logging event data. */
		  	  NULL); 		/* No event info provided. */



Creating a listener
*******************

Modules that should receive events managed by the Event Manager must be registered as listeners and subscribe to a given event type.
Every listener is identified by a unique name.

To turn a module into a listener for specific event types, include the header files for the respective event types, for example, ``#include "sample_event.h"``.
You must then implement an `Event handler function`_ and define the module as listener with the :c:macro:`EVENT_LISTENER` macro, passing the name of the module and the event handler function as arguments.
Finally, subscribe the listener to specific event types.

For subscribing to an event type, the Event Manager provides three types of subscriptions, differing in priority.
They can be registered with the following macros:

* :c:macro:`EVENT_SUBSCRIBE_EARLY` - notification before other listeners
* :c:macro:`EVENT_SUBSCRIBE` - standard notification
* :c:macro:`EVENT_SUBSCRIBE_FINAL` - notification as last, final subscriber

There is no defined order in which subscribers of the same priority are notified.

The module will receive events for the subscribed event types only.
The listener name passed to the subscribe macro must be the same as in :c:macro:`EVENT_LISTENER`.


Event handler function
======================

The event handler function is called when any of the subscribed event types is being processed.
Note that only one event handler function can be registered for a listener.
Therefore, if a listener subscribes to multiple event types, the function must handle all of them.

The event handler gets a pointer to the :cpp:class:`event_header` structure as function argument.
The function should return ``true`` to consume the event (which means that the event is not propagated to further listeners), or ``false`` otherwise.

To check if an event has a given type, call the function with the name is\_\ *event_type_name* (for example, ``is_sample_event()``), passing the pointer to the event header as argument.
This function returns ``true`` if the event matches the given type, or ``false`` otherwise.

To access the event data, cast the :cpp:class:`event_header` structure to a proper event type using the function with the name cast\_\ *event_type_name* (for example, ``cast_sample_event()``), passing the pointer to the event header as argument.

Code example
============

The following code example shows how to register an event listener with an event handler function and subscribe to the event type ``sample_event``:

.. code-block:: c

	#include "sample_event.h"

        static bool event_handler(const struct event_header *eh)
	{
		if (is_sample_event(eh)) {

			/* Accessing event data. */
			struct sample_event *event = cast_sample_event(eh);

			s8_t v1 = event->value1;
			s16_t v2 = event->value2;
			s32_t v3 = event->value3;

			/* Actions when received given event type. */
			foo(v1, v2, v3);

			return false;
		}

		return false;
	}

        EVENT_LISTENER(sample_module, event_handler);
	EVENT_SUBSCRIBE(sample_module, sample_event);



Profiling an event
******************

Event Manager events can be profiled (see :ref:`profiler`).
To profile a given Event Manager event, you must define an :cpp:class:`event_info` structure (with :c:macro:`EVENT_INFO_DEFINE`) and provide it as argument when defining the event type.
This structure contains a profiling function and information about the data fields that are logged.

The profiling function should log the event data to a given buffer by calling :cpp:func:`profiler_log_encode_u32` (regardless of the profiled data type).

The following code examples shows a profiling function for the event type ``sample_event``:

.. code::

	static void profile_sample_event(struct log_event_buf *buf,
					 const struct event_header *eh)
	{
		struct sample_event *event = cast_sample_event(eh);

		/* ARG_UNUSED to suppress warning when profiling is disabled. */
		ARG_UNUSED(event);
		profiler_log_encode_u32(buf, event->value1);
		profiler_log_encode_u32(buf, event->value2);
		profiler_log_encode_u32(buf, event->value3);
	}

The following code example shows how to define the event profiling information structure and add it to event type definition:

.. code::

	EVENT_INFO_DEFINE(sample_event,
			  /* Profiled datafield types. */
			  ENCODE(PROFILER_ARG_S8, PROFILER_ARG_S16, PROFILER_ARG_S32),
			  /* Profiled data field names - displayed by profiler. */
			  ENCODE("value1", "value2", "value3"),
			  /* Function used to profile event data. */
			  profile_sample_event);

	EVENT_TYPE_DEFINE(sample_event,
			  true,
			  log_sample_event, 	/* Function for logging event data. */
			  &sample_event_info); 	/* Structure with data for profiling. */

.. note::
	By default, all Event Manager events that are defined with an :cpp:class:`event_info` argument are profiled.

Shell integration
*****************

The Event Manager is integrated with Zephyr's :ref:`zephyr:shell_api` module.
When the shell is turned on, an additional subcommand set (:command:`event_manager`) is added.

This subcommand set contains the following commands:

:command:`show_listeners`
  Show all registered listeners.

:command:`show_subscribers`
  Show all registered subscribers.

:command:`show_events`
  Show all registered event types.
  The letters "E" or "D" indicate if logging is currently enabled or disabled for a given event type.

:command:`enable` or :command:`disable`
  Enable or disable logging.
  If called without additional arguments, the command applies to all event types.
  To enable or disable logging for specific event types, pass the event type indexes (as displayed by :command:`show_events`) as arguments.


API documentation
*****************

| Header file: :file:`include/event_manager.h`
| Source files: :file:`subsys/event_manager/`

.. doxygengroup:: event_manager
   :project: nrf
   :members:
