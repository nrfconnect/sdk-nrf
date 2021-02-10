.. _lib_caf:

Common Application Framework
############################

Common Application Framework (CAF) is a set of ready-for-use modules built on top of :ref:`event_manager`.
You can use CAF to have a consistent event-based architecture in your application.

In an event-based application, parts of the application functionality are separated into isolated modules that communicate with each other using events.
Events are specified by each module and the application modules can subscribe and react to them.

One of the applications that are using the CAF modules in |NCS| is :ref:`nrf_desktop`.
For details about events and their implementation, see the :ref:`event_manager` page.
