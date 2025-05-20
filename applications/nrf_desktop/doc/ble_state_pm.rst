.. _nrf_desktop_ble_state_pm:

Bluetooth state power manager module
####################################

.. contents::
   :local:
   :depth: 2

In nRF Desktop, the Bluetooth® state power manager module is responsible for blocking power off when there is at least one active Bluetooth® connection.

For more information about the Bluetooth configuration in the nRF Desktop, see the :ref:`nrf_desktop_bluetooth_guide` documentation.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_state_pm_start
    :end-before: table_ble_state_pm_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

nRF Desktop uses the |ble_state_pm| from :ref:`lib_caf` (CAF).
See the :ref:`CAF Bluetooth state power manager module <caf_ble_state_pm>` page for implementation details.
