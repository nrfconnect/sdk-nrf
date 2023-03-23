.. _event_manager_proxy:

Event Manager Proxy
###################

.. contents::
   :local:
   :depth: 2

The Event Manager Proxy is a library that allows to exchange Event Manager events between cores.
It connects two separate instances of Event Manager on different cores by passing registered messages using the IPC service.

See the :ref:`event_manager_proxy_sample` sample for an example of how to use this library.

Configuration
*************

To use the Event Manager Proxy, enable the :kconfig:option:`CONFIG_EVENT_MANAGER_PROXY` Kconfig option.
This option depends on :kconfig:option:`CONFIG_IPC_SERVICE` Kconfig option.
Make sure that the IPC Service is configured together with the used backend.

When Event Manager Proxy is enabled, the required hooks in :ref:`app_event_manager` are also enabled.

Additional configuration
========================

You can also set the following Kconfig options when working with Event Manager Proxy:

* :kconfig:option:`CONFIG_EVENT_MANAGER_PROXY_CH_COUNT` - This Kconfig sets the number of IPC instances that would be used.
  This option is related to the number of cores between which the events are exchanged.
  For example, having two cores means that there is one exchange taking place, and so you need one IPC instance.
* :kconfig:option:`CONFIG_EVENT_MANAGER_PROXY_BOND_TIMEOUT_MS` - This Kconfig sets the timeout value of the bonding.

Implementing the proxy
======================

When compiling the code that uses Event Manager Proxy, make sure that the event definitions shared between the cores are compiled for both cores.
The declarations should be accessible during code compilation of both cores.

The application that wishes to use Event Manager Proxy requires a special initialization process.
To implement an Event Manager Proxy, you must complete the following steps:

1. Initialize Event Manager Proxy together with the :ref:`app_event_manager`.
   The code should look as follows:

    .. code-block:: c

       /* Initialize Event Manager and Event Manager Proxy */
       ret = event_manager_init();
       /* Error handling */

#. Add all remote IPC instances.
   The code should look as follows:

    .. code-block:: c

       ret = event_manager_proxy_add_remote(ipc1_instance);
       /* Error handling */
       ret = event_manager_proxy_add_remote(ipc2_instance);
       /* Error handling */

#. Use :c:func:`event_manager_proxy_subscribe` function by passing all the required arguments for the submitter to pass the selected event.
   The auxiliary macro :c:macro:`EVENT_MANAGER_PROXY_SUBSCRIBE` prepares all the argument identifiers using the event definition.
   The code should look as follows:

    .. code-block:: c

       #include <event1_definition_file.h>
       #include <event2_definition_file.h>

       ret = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc1_instance, event1);
       /* Error handling */
       ret = EVENT_MANAGER_PROXY_SUBSCRIBE(ipc2_instance, event2);
       /* Error handling */

#. Configure the Event Manager Proxy into an active state when all the events are subscribed.

    .. code-block:: c

       ret = event_manager_proxy_start();
       /* Error handling */

   Now no more configuration messages should be passed between cores.
   Only subscribed events are transmitted.
#. Use :c:func:`event_manager_proxy_start` function on both the cores to transmit the events between both cores.
   If you want to be sure that the link between cores is active before continuing, use the :c:func:`event_manager_proxy_wait_for_remotes` function.
   This function blocks until all registered instances report their readiness.

A call to :c:func:`event_manager_proxy_wait_for_remotes` is not required.
The function is used if you want to send an event that is to be transmitted to all the registered remotes.

After the link is initialized, the subscribed events from the remote core appear in the local event queue.
The events are then processed like any other local messages.

Implementation details
**********************

The proxy uses some of the Event Manager hooks to connect with the manager.

Initialization hook usage
=========================

.. include:: app_event_manager.rst
   :start-after: em_initialization_hook_start
   :end-before: em_initialization_hook_end

The Event Manager Proxy uses the hook to append itself to the initialization procedure.

Tracing hook usage
==================

.. include:: app_event_manager.rst
   :start-after: em_tracing_hooks_start
   :end-before: em_tracing_hooks_end

The Event Manager Proxy uses only a post-process hook to send the event after processing the local core to the remote core that is registered as a listener.

Subscribing to remote events
============================

A core that wishes to listen to events from the remote core sends ``SUBSCRIBE`` command to that core during the initialization process.
The ``SUBSCRIBE`` command is sent using the :c:func:`event_manager_proxy_subscribe` function, which passes the following arguments:

* ``ipc`` - This argument is an IPC instance that identifies the communication channel between the cores.
* ``local_event_id`` - This argument represents the local core ID that is received when the event is post-processed on the remote core.
  It is also used to get the event name to match the same event on the remote core.

The remote core during the command processing searches for an event with the given name and registers the given event ID in an array of events.
The created array of events directly reflects the array of event types.
This way, the complexity of searching the remote event ID connected to the currently processed event has ``O(1)`` complexity.
The most time consuming search is realized during initialization, where events are searched by name with ``O(N)`` complexity.

Sending the event to the remote core
====================================

After the event is processed locally, the event post-process hook in the Event Manager Proxy is executed.
The proxy gets the event index and then checks the matched position in the remote array.
If the event is registered for this event for any of added remote IPC instance, the event is copied as-is.
The event ID is replaced by the ID requested by the remote and is transmitted to the remote in the same form.
This way, the remote can copy the event as-is and use the event as the remote's local event.

Passing the event from the remote core
======================================

Once the remote and local core started Event Manager Proxy by calling the :c:func:`event_manager_proxy_start` function, every piece of incoming data is treated as a single event.
A new event is allocated by :c:func:`event_manager_alloc` function and the event is submitted to the event queue by the :c:func:`_event_submit` function.
From that moment, the event is treated similarly as any other locally generated event.

.. note::
   If any of the shared events between the cores provide any kind of memory pointer, the pointed memory must be available for the target core if the core is to access the shared events.

Limitations
***********

The event passed through the Event Manager Proxy is treated and processed the same way as any locally generated event.
The core that sources the event must not subscribe to the same event in another core.
If it does, once the core receives such an event generated remotely, it would resend the event automatically from that local core to the ones that subscribed to it.
We would run into a dangerous situation where two cores subscribe to the same event.
Once generated, such an event would be continuously resent between the cores.
The currently proposed approach is to create special events for each core even if they look the same, they must have different codes.

.. _event_manager_proxy_api:

API documentation
*****************

| Header file: :file:`include/event_manager_proxy.h`
| Source files: :file:`subsys/event_manager_proxy/`

.. doxygengroup:: event_manager_proxy
   :project: nrf
   :members:
