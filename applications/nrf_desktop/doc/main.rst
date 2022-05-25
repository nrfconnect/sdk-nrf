.. _nrf_desktop_main:

Basic module (main)
###################

.. contents::
   :local:
   :depth: 2

The main module is the entry point of the nRF Desktop application.

Module events
*************

The module does not register itself as an :ref:`app_event_manager` listener.
The module only submits the ``module_state_event`` to inform the other modules that they can continue the application startup.

+-------------------+---------------+-------------+------------------------+---------------------------------------------+
| Source Module     | Input Event   | This Module | Output Event           | Sink Module                                 |
+===================+===============+=============+========================+=============================================+
| -                 | -             | ``main``    | ``module_state_event`` | :ref:`nrf_desktop_module_state_event_sinks` |
+-------------------+---------------+-------------+------------------------+---------------------------------------------+

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module has no configuration options.


Implementation details
**********************

The :ref:`nrf_desktop_main` initializes the :ref:`app_event_manager` and then submits the ``module_state_event`` to inform the other modules that they can continue the application startup.
