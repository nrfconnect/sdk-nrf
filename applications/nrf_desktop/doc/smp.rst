.. _nrf_desktop_smp:
.. _nrf_desktop_ble_smp:

Simple Management Protocol module
#################################

.. contents::
   :local:
   :depth: 2

The |smp| is responsible for performing a Device Firmware Upgrade (DFU) over Bluetooth® LE.
You can perform the DFU using, for example, the `nRF Connect for Mobile`_ application.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_smp_start
    :end-before: table_smp_end

.. note::
    |nrf_desktop_module_event_note|

Implementation details
**********************

nRF Desktop uses the |smp| from :ref:`lib_caf` (CAF).
See the :ref:`CAF module <caf_ble_smp>` page for the implementation details and guide on how to perform the firmware update in the `nRF Connect Device Manager`_ application.
