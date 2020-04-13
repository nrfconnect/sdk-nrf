.. _nrf_desktop_main:

Main module
###########

The main module is the entry point of the nRF Desktop application.


Module Events
*************

The module does not register itself as :ref:`event_manager` listener.
The module only submits the ``module_state_event`` to inform the other modules that they can continue the application startup.

See the :ref:`nrf_desktop_architecture` for more information about the event-based communication in the nRF Desktop application and about how to read this table.

Configuration
*************

The module has no configuration options.


Implementation details
**********************


The :ref:`nrf_desktop_main` module initializes the :ref:`event_manager` and then submits the ``module_state_event`` to inform the other modules that they can continue the application startup.
