.. _secondary_boot_sample:

Secondary Boot Sample
#####################

.. contents::
   :local:
   :depth: 2

This sample demonstrates booting a secondary application image on the nRF54H20DK board, where both the primary and secondary application images print "Hello World" messages.

Overview
********

The sample consists of two applications:

* **Primary Image**: Runs initially on the application core (cpuapp), prints a hello world message, and includes stub functions for:

  * Secure domain service calls to boot the secondary image

* **Secondary Image**: Runs on the same application core (cpuapp/secondary) after the primary image initiates the boot sequence and prints its own hello world message.

Requirements
************

* nRF54H20DK board

Building and Running
********************

To build the sample for nRF54H20DK:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/secondary_boot --sysbuild

The ``--sysbuild`` flag is required as this sample builds multiple images.

To flash the sample:

.. code-block:: console

   west flash

Sample Output
*************

When the sample runs successfully, you should see output similar to the following:

.. code-block:: console

   [00:00:00.123,456] === Hello World from Primary Image ===
   [00:00:00.456,789] === Hello World from Secondary Image ===
