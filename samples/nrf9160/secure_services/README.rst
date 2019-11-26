.. _secure_services:

nRF9160: Secure Services Sample
###############################

The Secure Services sample shows how to use the secure services provided by :ref:`secure_partition_manager`.
This firmware needs :ref:`secure_partition_manager` to also be present on the chip.

Overview
********

This sample generates random numbers and prints them to the console, sleeps, then reboots.
This is to demonstrate the :cpp:func:`spm_request_system_reboot` and :cpp:func:`spm_request_random_number` secure services.

Requirements
************

The following development board:

* |nRF9160DK|

* .. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/secure_services`

.. include:: /includes/build_and_run_nrf9160.txt



Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_spm`
