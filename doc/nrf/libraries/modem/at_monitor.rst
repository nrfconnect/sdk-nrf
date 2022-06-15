.. _at_monitor_readme:

AT monitor
##########

.. contents::
   :local:
   :depth: 2

The AT monitor library is used to define AT monitors, which consist of a handler function that receives AT notifications from the :ref:`nrfxlib:nrf_modem`, based on a filter.
The handler is executed from the system workqueue or from an ISR, depending on the type of monitor.

Overview
========

The Modem library :ref:`nrfxlib:nrf_modem_at` supports only a single handler function that can receive AT notifications.
The handler is executed in an interrupt service routine (ISR).
It is likely that multiple parts of the application might require AT notifications, and that the processing of the notification is done outside an ISR.

The AT monitor library facilitates the integration of the Modem library in |NCS| applications in two ways:

#. It handles the copying of notifications and reschedules their processing in a thread.
#. It dispatches notifications to multiple parts of the application.

.. note::
   When the AT monitor library is enabled, the application must not call :c:func:`nrf_modem_at_notif_handler_set` as that will override the handler set by the AT monitor library.
   The application must instead define an AT monitor with a filter set to :c:macro:`ANY` to achieve the same behavior of registering a handler for AT notifications with the Modem library.

Initialization
==============

The AT monitor library initializes automatically when enabled using :kconfig:option:`CONFIG_AT_MONITOR`.
Upon initialization, the AT monitor library registers itself as the receiver of AT notifications from the Modem library (using :c:func:`nrf_modem_at_notif_handler_set`).

.. note::
   When the AT monitor library is initialized, it will override the handler set previously with the Modem library to receive the AT notifications.

Usage
=====

The application can define an AT monitor to receive notifications through the :c:macro:`AT_MONITOR` and :c:macro:`AT_MONITOR_ISR` macros.
An AT monitor has a name, a filter string, and a callback function.
In addition, it can be paused or activated (using :c:macro:`PAUSED` and :c:macro:`ACTIVE` respectively).
Multiple parts of the application can define their own AT monitor with the same filter as another AT monitor, and thus receive the same notifications, if desired.

Deferred dispatching
********************

The application can define an AT monitor to receive AT notifications in the system workqueue using the :c:macro:`AT_MONITOR` macro.
When the AT monitor library receives an AT notification from the Modem library, the notification is copied on the AT monitor library heap and is dispatched using the system workqueue to all monitors whose filter matches (even partially) the contents of the notification.

The following code snippet shows how to register a handler that receives ``+CEREG`` notifications from the Modem library:

.. code-block:: c

	/* AT monitor for +CEREG notifications */
	AT_MONITOR(network_registration, "+CEREG", cereg_mon);

	int cereg_mon(const char *notif)
	{
		printf("Received +CEREG notification: %s", notif);
	}

The size of the AT monitor library heap can be configured using the :kconfig:option:`CONFIG_AT_MONITOR_HEAP_SIZE` option.

Direct dispatching
******************

The AT monitor library supports defining a particular type of monitor that receives the AT notifications in an interrupt service routine.
Because notifications dispatched to AT monitors in an ISR are not copied onto the AT monitor library heap, the application is guaranteed that the library will not be out of memory to copy the notification.
This can be useful for some particularly large AT notifications or AT notifications that the application must reply to, for example, SMS notifications.

The following code snippet shows how to register a handler that receives ``+CEREG`` notifications from the Modem library:

.. code-block:: c

	/* AT monitor for +CEREG notifications, dispatched in ISR */
	AT_MONITOR_ISR(network_registration, "+CEREG", cereg_mon);

	int cereg_mon(const char *notif)
	{
		printf("Received +CEREG notification in ISR");
	}

Pausing and resuming
********************

When defined, an AT monitor is in active state by default.
An AT monitor can be paused and resumed with the :c:func:`at_monitor_pause` and :c:func:`at_monitor_resume` functions respectively.
If desired, an AT monitor can be defined to be in paused state at compile time by appending :c:macro:`PAUSED` to the monitor definition.

The following code snippet shows how to define an AT monitor for ``+CEREG`` notifications that is paused at boot and resumed later:

.. code-block:: c

	/* AT monitor for +CEREG notifications, paused until manually activated */
	AT_MONITOR(network_registration, "+CEREG", cereg_mon, PAUSED);

	void foo(void)
	{
		/* let's resume the monitor */
		at_monitor_resume(&network_registration);
	}

Wildcard filter
***************

It is possible to define an AT monitor that will receive all AT notifications, by passing :c:macro:`ANY` as the AT monitor filter string.

The following code snippet shows how to define an AT monitor that will receive all AT notifications:

.. code-block:: c

	/* AT monitor for all notifications */
	AT_MONITOR(catch_all, ANY, at_notif_handler);

	int at_notif_handler(const char *notif)
	{
		printf("Received a notification: %s", notif);
	}

API documentation
=================

| Header file: :file:`include/modem/at_monitor.h`
| Source file: :file:`lib/at_monitor/at_monitor.c`

.. doxygengroup:: at_monitor
   :project: nrf
   :members:
