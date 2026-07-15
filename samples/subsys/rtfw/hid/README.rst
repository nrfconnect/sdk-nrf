.. _rtfw_hid_sample:

.. ncs-sample::
   :title: RTFW HID/GPIOTE source adapter

   The HID sample demonstrates the fast-capture model described in :ref:`lib_rtfw`.
   An owned GPIOTE channel timestamps and normalizes input edges in the source ZLI, and RTFW delivers the events to Zephyr, where they become Bluetooth HIDS mouse reports.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a Bluetooth LE-capable host that supports HID over GATT.

Board-specific resources
========================

The sample uses the following hardware resources on each supported development kit.

nRF54LM20 DK
-------------

* GPIOTE20 is the direct interrupt source.
* EGU20 is the RT-to-Zephyr doorbell.
* **P1.26**, connected to **Button 0**, is the active-low, pull-up input.
* **P1.07** is the optional ISR debug output.
* GPIOTE20 and EGU20 are disabled in devicetree so no Zephyr driver claims them.

nRF54H20 DK
------------

* GPIOTE130 is the direct interrupt source.
* EGU130 is the RT-to-Zephyr doorbell.
* **P0.08**, connected to **Button 0**, is the active-low, pull-up input.
* **P0.03** is the optional ISR debug output.
* GPIOTE130 and EGU130 are reserved for the application core.

The board configurations select ZLI priorities according to the requirements in :ref:`lib_rtfw_interrupt_priority`.
The sample sets :kconfig:option:`CONFIG_GPIO` to ``n``, which disables the Zephyr GPIO subsystem, and configures the input and GPIOTE channel through Nordic HAL calls.
See the SoC reservations before changing an instance or pin.

Overview
********

The sample uses the following input path:

.. code-block:: text

   GPIO edge
      -> GPIOTE source ZLI
      -> RTFW event delivery
      -> bt_hids_inp_rep_send()

The GPIOTE ISR processes configuration commands before hardware events, allowing Zephyr to control input capture through the same owned source IRQ.
The sample enables capture during startup and does not expose a runtime control for disabling it.
Enabling capture clears stale events, while repeated enable requests preserve a real pending edge.
Disabling capture masks the source and discards any pending event.

Input polarity and pull configuration come from the ``rt-input-gpios`` devicetree flags.
The supplied overlays select active-low with an internal pull-up; requesting pull-up and pull-down together is rejected at build time.

Bluetooth and settings
======================

It requests Bluetooth security level 2 after connecting.
HIDS connection state is registered and released through ``bt_hids_connected()`` and ``bt_hids_disconnected()``.
Input edges captured without an active Bluetooth connection are discarded.

Bluetooth identity, bonding data, CCC state, and the GATT database hash use the settings subsystem with a ZMS backend.
On first boot, the Bluetooth host can print that no identity address exists before ``settings_load()`` runs.
After the identity has been stored, it should remain stable when the board reboots.

Limitations
===========

The sample intentionally leaves debouncing, gestures, and application policy outside the ZLI.
A bouncing input can produce multiple HID events, while closely spaced edges can coalesce before servicing.
A sustained event storm can overflow the RTFW queue or delay HIDS submission; both conditions are reported by the benchmark.
The sample does not support system PM and rejects :kconfig:option:`CONFIG_PM` at build time.

User interface
**************

The sample advertises as ``Nordic_RTFW_HID`` with the HID service UUID and mouse appearance.

**Button 0**:
   Pressing the button sends a mouse report with mouse button 1 asserted and X movement of 8.
   Releasing the button sends a neutral report.

Configuration
*************

|config|

.. options-from-kconfig::
   :show-type:

Latency instrumentation
=======================

Enable :option:`CONFIG_SAMPLE_RTFW_DEBUG_PIN` to assert the board-specific debug output around the direct source ISR.
Measuring the input and debug output together separates:

* Edge-to-ZLI entry latency
* Source ISR execution time
* The later ZLI-capture-to-HIDS-submit metric printed by the sample

The :option:`CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD` option adds a controlled NOP loop to the source ISR.
Use the :option:`CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD_CYCLES` option to select its size.
This option is for stress testing and should remain disabled for baseline measurements.

Building and running
********************

.. |sample path| replace:: :file:`samples/subsys/rtfw/hid`

.. include:: /includes/build_and_run.txt

Command-line builds
===================

Run the following command depending on your development kit:

.. tabs::

   .. group-tab:: nRF54LM20 DK

      Run the following commands:

      .. code-block:: console

         west build -b nrf54lm20dk/nrf54lm20b/cpuapp \
           nrf/samples/subsys/rtfw/hid
         west flash

   .. group-tab:: nRF54H20 DK

      Run the following commands to build the sysbuild configuration:

      .. code-block:: console

         west build --sysbuild -b nrf54h20dk/nrf54h20/cpuapp \
           nrf/samples/subsys/rtfw/hid
         west flash

The nRF54H20 DK build enables the radio core and includes the ``ipc_radio`` child image.
The application image owns RTFW and HIDS, while the radio image provides the Bluetooth controller over HCI IPC.

Use a separate build directory or a pristine build when switching targets or variants.

Testing
========

|test_sample|

Mouse input
-----------

Complete the following steps on the nRF54LM20 DK or the nRF54H20 DK:

#. Flash the sample.
#. |connect_terminal|
#. Reset the development kit.
#. Wait for ``HID advertising start: 0``.
#. Find ``Nordic_RTFW_HID`` in the host operating system's Bluetooth settings.
#. Pair and connect it as a mouse.
#. Wait for security and HID notification setup to complete.
#. Press and release **Button 0**.
#. Verify that the host receives mouse button and X-movement reports.

If a host retains incompatible bonding information from an older build, remove the device from the host and erase the board's settings before pairing again.

Measuring delivery latency
--------------------------

The benchmark measures from a timestamp captured in the source ZLI to the return of ``bt_hids_inp_rep_send()``.
It includes RTFW delivery, workqueue scheduling, and time spent in the HIDS submission call, but excludes edge-to-ZLI latency, over-air delivery, and host processing.
Unsigned 32-bit subtraction remains correct across one counter wrap, provided one measured interval is shorter than about 71 minutes.

After every 128 input events processed while connected, the sample prints a report that combines per-window latency and HIDS submission statistics with cumulative RTFW diagnostics.

.. code-block:: console

   HID edge->HIDS submit us: avg=235 median_lb=232 min=123 max=581 \
     jitter=458 hist_overflows=0 hids_window_drops=0 last_hids_err=0 \
     rtfw_drops=0 rtfw_max_depth=1 rtfw_faults=0x00000000 \
     rtfw_status_err=0

The fields are:

.. list-table::
   :header-rows: 1
   :widths: 20 15 65

   * - Field
     - Scope
     - Description
   * - ``avg``
     - Per-window
     - Arithmetic mean in microseconds
   * - ``median_lb``
     - Per-window
     - Lower bound of the median histogram bucket
   * - ``min`` and ``max``
     - Per-window
     - Exact minimum and maximum in the window
   * - ``jitter``
     - Per-window
     - Difference between ``max`` and ``min``
   * - ``hist_overflows``
     - Per-window
     - Samples at or beyond the histogram range
   * - ``hids_window_drops``
     - Per-window
     - Failed HIDS API submissions in this 128-event window, not over-air packet loss
   * - ``last_hids_err``
     - Per-window
     - Most recent HIDS submission error in the window
   * - ``rtfw_drops``
     - Cumulative
     - Framework queue overflows since initialization
   * - ``rtfw_max_depth``
     - Cumulative
     - Queue high-water mark, or zero when :kconfig:option:`CONFIG_RTFW_QUEUE_USAGE_STATS` is disabled
   * - ``rtfw_faults``
     - Cumulative
     - Framework fault bits
   * - ``rtfw_status_err``
     - Current report
     - Result of reading the framework status snapshot

The histogram has 256 buckets, each 8 µs wide, and represents a range of 2048 µs.
Values at or above 2048 µs saturate in the final bucket and increment ``hist_overflows``.
The per-window latency statistics, HIDS submission errors, and histogram contents are reset after every report.
The RTFW drop count, queue high-water mark, and fault bits remain cumulative from framework initialization.
The sample sets :kconfig:option:`CONFIG_RTFW_WORKQ_STACK_SIZE` to ``3072`` because the delivery callback enters the HIDS path and formats 64-bit latency output.

Validating latency measurements
-------------------------------

For a representative test:

#. Record the board, build configuration, controller version, connection parameters, and input stimulus.
#. Pair and establish an encrypted HID connection.
#. Generate repeatable input edges and capture several statistics windows and debug-pin traces.
#. Verify that ``hids_window_drops`` and ``rtfw_drops`` remain zero under the intended load, including during unrelated Zephyr activity and radio traffic.
#. Record ``hist_overflows`` and compare it with the baseline for the same target and test conditions.
   A non-zero value indicates that some ZLI-to-HIDS-submit latencies exceeded 2048 µs.
   It does not indicate an RTFW event loss.
   On the nRF54H20 DK, such overflows can occur while HIDS submission waits for controller communication over IPC.
#. Compare against an equivalent Zephyr GPIO-callback implementation.
#. Verify disconnect, re-advertising, re-pairing, and settings persistence.

Measure the delivery stack high-water mark while exercising connection and security setup, HIDS notifications, latency formatting, and an edge burst.
For example, make an instrumentation build with :kconfig:option:`CONFIG_THREAD_ANALYZER` and :kconfig:option:`CONFIG_THREAD_ANALYZER_AUTO` enabled and :kconfig:option:`CONFIG_THREAD_ANALYZER_AUTO_INTERVAL` set to ``5``, then verify that ``rtfw_delivery`` retains an adequate margin.

Dependencies
************

This sample uses the following |NCS| components:

* :ref:`lib_rtfw`
* :ref:`hids_readme`
* :ref:`ipc_radio` child image on the nRF54H20 DK

It also uses the following Zephyr components:

* :ref:`zephyr:bluetooth_api`
* :ref:`zephyr:settings_api`
* :ref:`zephyr:zms_api`

The sample also uses hardware abstraction layers from `nrfx`_.
