.. _wdtstart_sample:

WDTSTART sample
###############

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to configure UICR.WDTSTART to automatically start a watchdog timer before the application core is booted on the nRF54H20 DK.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample demonstrates the UICR.WDTSTART feature, which allows automatic watchdog timer startup before the application core boots.
This provides early system protection ensuring that the system can recover from early boot failures.

The sample shows how to:

* Enable UICR.WDTSTART configuration through sysbuild
* Configure watchdog timer instance (WDT0 or WDT1)
* Set the initial Counter Reload Value (CRV)
* Detect watchdog resets using the hwinfo driver
* Feed the watchdog to prevent further resets

Behavior
========

On first boot (or after a non-watchdog reset):

* The sample prints reset cause information
* It then waits for the watchdog to trigger a reset (without feeding it)

After a watchdog reset:

* The sample detects the watchdog reset using hwinfo
* It continuously feeds the watchdog to prevent further resets
* This demonstrates successful watchdog reset detection and handling

Building and running
********************

.. |sample path| replace:: :file:`samples/ironside_se/wdtstart_sample`

.. include:: /includes/build_and_run_ns.txt

To build the sample for the nRF54H20 DK, run the following command:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp samples/ironside_se/wdtstart_sample

To program the sample on the device, run the following command:

.. code-block:: console

   west flash

Configuration
*************

The watchdog configuration is set in :file:`sysbuild/uicr.conf`:

* ``CONFIG_GEN_UICR_WDTSTART`` - Enable UICR.WDTSTART
* ``CONFIG_GEN_UICR_WDTSTART_INSTANCE_WDT1`` - Use WDT1 instance
* ``CONFIG_GEN_UICR_WDTSTART_CRV`` - Counter Reload Value (65535 = ~2 seconds)

Testing
*******

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Observe the console output:

   * First boot shows no watchdog reset detected
   * The sample waits for watchdog to trigger
   * After ~2 seconds, the watchdog triggers a reset
   * On second boot, the sample detects the watchdog reset
   * The sample continuously feeds the watchdog to prevent further resets

Expected output:

.. code-block:: console

   === IronSide SE WDTSTART Sample ===

   Reset cause: 0x00000000
   No reset detected (normal boot)

   No watchdog reset detected - waiting for watchdog to trigger...
   The watchdog should timeout and reset the system shortly.
   After the reset, this sample will detect the watchdog reset
   and continuously feed the watchdog to prevent further resets.

   Waiting for watchdog timeout (not feeding watchdog)...
   Still waiting for watchdog to trigger...
   
   [System resets after ~2 seconds]
   
   === IronSide SE WDTSTART Sample ===

   Reset cause: 0x00000002
   *** WATCHDOG RESET DETECTED ***

   Watchdog reset detected - continuously feeding watchdog
   This demonstrates that we can detect and handle watchdog resets

   Watchdog driver initialized successfully
   Watchdog fed 1 times - system still running!
   Watchdog fed 501 times - system still running!
   Watchdog fed 1001 times - system still running!

Dependencies
************

This sample uses the following |NCS| subsystems:

* UICR generation - Configures the User Information Configuration Registers for WDTSTART

In addition, it uses the following Zephyr subsystems:

* :ref:`zephyr:kernel_api` - Provides basic system functionality and threading
* :ref:`zephyr:console_api` - Enables UART console output for debugging and user interaction
* :ref:`zephyr:watchdog_api` - Watchdog timer driver interface
* :ref:`zephyr:hwinfo_api` - Hardware information and reset cause detection
