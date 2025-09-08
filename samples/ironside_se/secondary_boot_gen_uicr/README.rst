.. _secondary_boot_sample:

Secondary boot
##############

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to boot a secondary application image on the nRF54H20 DK, where both the primary and secondary application images print ``Hello World`` messages.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample consists of two applications:

* *Primary Image*: Runs initially on the application core (``cpuapp``), prints a ``Hello World`` message, and includes stub functions for the Secure Domain service calls to boot the secondary image.
* *Secondary Image*: Runs on the same application core after the primary image initiates the boot sequence and prints its own ``Hello World`` message.

Building and running
********************

.. |sample path| replace:: :file:`samples/ironside_se/secondary_boot_gen_uicr`

.. include:: /includes/build_and_run_ns.txt

To build the sample for the nRF54H20 DK, run the following command:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/secondary_boot

To program the sample on the device, run the following command:

.. code-block:: console

   west flash

Testing
*******

After programming the test to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Observe the console output for both cores:


.. code-block:: console

   [00:00:00.123,456] === Hello World from Primary Image ===
   [00:00:00.456,789] === Hello World from Secondary Image ===

Dependencies
************

This sample uses the following |NCS| subsystems:

* IronSide SE bootmode service - Provides the interface to boot into secondary firmware mode
* Sysbuild - Enables building multiple images in a single build process
* UICR generation - Configures the User Information Configuration Registers for secondary boot mode

In addition, it uses the following Zephyr subsystems:

* :ref:`Kernel <kernel>` - Provides basic system functionality and threading
* :ref:`Console <console>` - Enables UART console output for debugging and user interaction
