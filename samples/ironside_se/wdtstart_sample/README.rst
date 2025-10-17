.. _wdtstart_sample:

WDTSTART
########

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to configure UICR.WDTSTART on the nRF54H20 DK to automatically start a watchdog timer before the application core is booted.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample demonstrates the UICR.WDTSTART feature, which starts a watchdog before the application core boots.
This provides early system protection to ensure that the system can recover from failures that occur during early boot.

The sample demonstrates how to do the following:

* Enable UICR.WDTSTART configuration through sysbuild.
* Configure watchdog timer instance (WDT0 or WDT1).
* Set the initial counter reload value (CRV).
* Detect watchdog resets using the hwinfo driver.
* Feed the watchdog to prevent further resets.

Operation
=========

The sample does the following:

1. On the first boot (or after a non-watchdog reset), the sample prints the reset cause information.
#. It then waits for the watchdog timer to trigger a reset without feeding it.
#. After the watchdog reset, the sample detects the watchdog reset using the hwinfo API.
#. It continuously feeds the watchdog to prevent further resets.
   This demonstrates the successful detection and handling of a watchdog reset.

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
* ``CONFIG_GEN_UICR_WDTSTART_CRV`` - Counter reload value (65535 = ~2 seconds)

Testing
*******

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Reset the kit.
#. Observe the following in the console output:

   * On the first boot, no watchdog reset is detected.
   * The sample waits for the watchdog timer to trigger.
   * After approximately 2 seconds, the watchdog triggers a system reset.
   * On the second boot, the sample detects the watchdog reset.
   * The sample then continuously feeds the watchdog to prevent further resets.

   The following is example of the expected console output:

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
* :ref:`zephyr:watchdog_api` - Watchdog timer driver interface
* :ref:`zephyr:hwinfo_api` - Hardware information and reset cause detection
