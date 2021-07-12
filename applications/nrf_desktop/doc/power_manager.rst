.. _nrf_desktop_power_manager:

Power manager module
####################

.. contents::
   :local:
   :depth: 2

Use the power manager module to reduce the energy consumption of battery-powered devices.
The module achieves this by switching to low power modes when the device is not used for a longer period of time.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_power_manager_start
    :end-before: table_power_manager_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

nrf Desktop uses the power manager module from :ref:`lib_caf` (CAF).
See the :ref:`CAF power manager module <caf_power_manager>` page for implementation details.
