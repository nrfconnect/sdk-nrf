.. _nrf_desktop_main:

Main module
###########

The main module is the entry point of the nRF Desktop application.


Module Events
*************

+------------------+-------------+--------------------------+------------------------+------------------+
| Source Module    | Input Event | This Module              | Output Event           | Sink Module      |
+==================+=============+==========================+========================+==================+
|                  |             |  :ref:`nrf_desktop_main` | ``module_state_event`` |                  |
+------------------+-------------+--------------------------+------------------------+------------------+

Configuration
*************

The module has no configuration options.


Implementation details
**********************


The :ref:`nrf_desktop_main` module initializes the :ref:`event_manager` and then submits the ``module_state_event`` to inform the other modules that they can continue the application startup.
