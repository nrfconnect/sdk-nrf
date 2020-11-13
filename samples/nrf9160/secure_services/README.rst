.. _secure_services:

nRF9160: Secure Services
########################

.. contents::
   :local:
   :depth: 2

The Secure Services sample shows how to use the secure services provided by :ref:`secure_partition_manager`.
This firmware needs :ref:`secure_partition_manager` to also be present on the chip.

Overview
********

This sample generates random numbers and prints them to the console, sleeps, then reboots.
This is to demonstrate the :c:func:`spm_request_system_reboot` and :c:func:`spm_request_random_number` secure services.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/secure_services`

.. include:: /includes/build_and_run_nrf9160.txt



Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_spm`
