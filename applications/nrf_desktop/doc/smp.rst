.. _nrf_desktop_smp:
.. _nrf_desktop_ble_smp:

Simple Management Protocol module
#################################

.. contents::
   :local:
   :depth: 2

The |smp| is responsible for performing a Device Firmware Upgrade (DFU) over Bluetooth® LE.

You can perform the DFU using for example the `nRF Connect for Mobile`_ application.
The :guilabel:`DFU` button appears in the tab with the connected Bluetooth® devices.
After pressing the button, you can select the :file:`*.bin` file that is to be used for the firmware update.

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
See the :ref:`CAF module <caf_ble_smp>` page for implementation details.

.. |smp| replace:: simple management protocol module
