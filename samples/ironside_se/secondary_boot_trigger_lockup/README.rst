.. _secondary_boot_trigger_lockup_sample:

Secondary boot with APPLICATIONLOCKUP trigger
##############################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates an automatic secondary boot triggered by a lockup of the application core CPU on the nRF54H20 DK.
When the primary application triggers a CPU lockup, IronSide SE automatically boots the secondary image without requiring any application-level software intervention.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample consists of two applications:

* *Primary Image*: Runs initially on the application core (``cpuapp``), prints status messages, and deliberately triggers a CPU lockup by disabling fault exceptions and then causing a fault.
* *Secondary Image*: Automatically boots after the lockup is detected and runs continuously.

The UICR.SECONDARY.TRIGGER.APPLICATIONLOCKUP configuration causes IronSide SE to automatically boot the secondary image when a CPU lockup is detected.

Configuration
*************

The sample uses the following UICR configuration in the :file:`sysbuild/uicr.conf` file:

.. code-block:: kconfig

   CONFIG_GEN_UICR_SECONDARY=y
   CONFIG_GEN_UICR_SECONDARY_TRIGGER=y
   CONFIG_GEN_UICR_SECONDARY_TRIGGER_APPLICATIONLOCKUP=y

This enables automatic secondary boot when the application core experiences a CPU lockup.

For more information about secondary firmware configuration and other available triggers, see :ref:`ug_nrf54h20_ironside_se_secondary_firmware`.

Building and running
********************

.. |sample path| replace:: :file:`samples/ironside_se/secondary_boot_trigger_lockup`

.. include:: /includes/build_and_run_ns.txt

To build the sample for the nRF54H20 DK, run the following command:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/ironside_se/secondary_boot_trigger_lockup

To program the sample on the device, run the following command:

.. code-block:: console

   west flash

Testing
*******

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Observe in the console output the primary image startup and lockup trigger:

   .. code-block:: console

      === Primary Image: Demonstrating APPLICATIONLOCKUP trigger ===
      This image will intentionally trigger a CPU lockup.
      The UICR.SECONDARY.TRIGGER.APPLICATIONLOCKUP configuration will
      automatically boot the secondary image.

      Triggering CPU lockup now!
      Step 1: Disabling fault exceptions and then accessing invalid memory...

#. Observe in the console output that the system automatically reboots into the secondary image:

   .. code-block:: console

      === Secondary Image: Successfully booted! ===
      The system automatically booted the secondary image due to
      APPLICATION LOCKUP in the primary image.

      This demonstrates UICR.SECONDARY.TRIGGER.APPLICATIONLOCKUP
      automatic failover capability.

      Secondary image heartbeat - system is stable
      Secondary image heartbeat - system is stable
      ...

Dependencies
************

This sample uses the following |NCS| subsystems:

* Sysbuild - Enables building multiple images in a single build process
* UICR generation - Configures the User Information Configuration Registers for automatic secondary boot on lockup

In addition, it uses the following Zephyr subsystems:

* :ref:`Kernel <kernel>` - Provides basic system functionality and threading
* :ref:`Console <console>` - Enables UART console output for debugging and user interaction
