.. _nrf_desktop_ble_state_pm:

Bluetooth state power manager module
####################################

.. contents::
   :local:
   :depth: 2

In nRF Desktop, the |ble_state_pm| is responsible for blocking power off when there is at least one active Bluetooth® connection.

Implementation details
**********************

nRF Desktop uses the |ble_state_pm| from :ref:`lib_caf` (CAF).
See the :ref:`CAF Bluetooth state power manager module <caf_ble_state_pm>` page for implementation details.

.. |ble_state_pm| replace:: Bluetooth state power manager module
