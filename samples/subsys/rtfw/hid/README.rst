.. _rtfw_hid_sample:

RTFW HID/GPIOTE source adapter
##############################

The HID sample demonstrates the fast-capture model described in
:ref:`rtfw_concept`. An owned GPIOTE channel timestamps and normalizes input
edges in the source ZLI. RTFW delivers the events to Zephyr, where they become
Bluetooth HIDS mouse reports.

Data flow
*********

The input path is:

.. code-block:: text

   GPIO edge
      -> GPIOTE source ZLI
      -> RTFW event delivery
      -> bt_hids_inp_rep_send()

The GPIOTE ISR consumes a pending configuration command before processing a
hardware event. This lets Zephyr enable or disable input capture by
software-pending the same owned source IRQ. The first disabled-to-enabled
transition clears a stale event before enabling capture. Repeating enable is a
no-op and does not clear a real pending edge. An enabled-to-disabled transition
first masks the source and then explicitly discards any pending event.

Input polarity and pull configuration come from the ``rt-input-gpios``
devicetree flags. The supplied overlays select active-low with an internal
pull-up; requesting pull-up and pull-down together is rejected at build time.
For the supplied mapping, a low level produces a mouse report with button 1
asserted and X movement of 8. Returning high sends a neutral report.

Hardware resources
******************

nRF54LM20 DK
============

* GPIOTE20 is the direct interrupt source.
* EGU20 is the RT-to-Zephyr doorbell.
* P1.26, DK button 0, is the active-low, pull-up input.
* P1.07 is the optional ISR debug output.
* GPIOTE20 and EGU20 are disabled in devicetree so no Zephyr driver claims
  them.

nRF54H20 DK
===========

* GPIOTE130 is the direct interrupt source.
* EGU130 is the RT-to-Zephyr doorbell.
* P0.08, DK button 0, is the active-low, pull-up input.
* P0.03 is the optional ISR debug output.
* GPIOTE130 and EGU130 are reserved for the application core.

The board configurations apply the target-specific ZLI priority policy
described in :ref:`rtfw_concept`. Compile-time assertions enforce the selected
priority and number of ZLI levels.

The sample disables the Zephyr GPIO driver and configures the input and GPIOTE
channel through Nordic HAL calls. Recheck SoC reservations before changing an
instance or pin.

Bluetooth and settings
**********************

The peripheral advertises as ``Nordic_RTFW_HID`` with the HID service UUID and
mouse appearance. It requests Bluetooth security level 2 after connecting.
HIDS connection state is registered and released through
``bt_hids_connected()`` and ``bt_hids_disconnected()``.

Bluetooth identity, bonding data, CCC state, and the GATT database hash use the
settings subsystem with a ZMS backend. On first boot, the Bluetooth host can
print that no identity address exists before ``settings_load()`` runs. The
identity should remain stable after it has been stored and the board reboots.

Building and flashing
*********************

Build for nRF54LM20 DK:

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20b/cpuapp \
     nrf/samples/subsys/rtfw/hid
   west flash

Build the nRF54H20 sysbuild configuration:

.. code-block:: console

   west build --sysbuild -b nrf54h20dk/nrf54h20/cpuapp \
     nrf/samples/subsys/rtfw/hid
   west flash

This build enables the radio core and includes the ``ipc_radio`` child image.
The application image owns RTFW and HIDS, while the radio image provides the
Bluetooth controller over HCI IPC. Inspect the sysbuild domains or build output
for both ``hid`` and ``ipc_radio`` before flashing.

Use a separate build directory or a pristine build when switching targets or
variants.

Running the sample
******************

On nRF54LM20 DK and nRF54H20 DK:

1. Flash the sample and wait for ``HID advertising start: 0``.
2. Find ``Nordic_RTFW_HID`` in the host operating system's Bluetooth settings.
3. Pair and connect it as a mouse.
4. Wait for security and HID notification setup to complete.
5. Press and release DK button 0.
6. Verify that the host receives mouse button and X-movement reports.

If a host retains incompatible bonding information from an older build, remove
the device from the host and erase the board's settings before pairing again.

Latency benchmark
*****************

The sample reads the lower 32 bits of the 1-MHz GRTC SYSCOUNTER in the source
ZLI and after ``bt_hids_inp_rep_send()`` returns. The result therefore includes
RTFW delivery, workqueue scheduling, and any time spent waiting inside the
HIDS submission call. Unsigned 32-bit subtraction remains correct across one
counter wrap, provided one measured interval is shorter than about 71 minutes.

It does not measure completion of a Bluetooth connection event, over-air
delivery, or host processing.

Every 128 connected-input events, the sample prints one independent statistics
window:

.. code-block:: console

   HID edge->HIDS submit us: avg=235 median_lb=232 min=123 max=581 \
     jitter=458 hist_overflows=0 hids_window_drops=0 last_hids_err=0 \
     rtfw_drops=0 rtfw_max_depth=1 rtfw_faults=0x00000000 \
     rtfw_status_err=0

The fields are:

* ``avg``: arithmetic mean in microseconds,
* ``median_lb``: lower bound of the median histogram bucket,
* ``min`` and ``max``: exact minimum and maximum in the window,
* ``jitter``: ``max - min``,
* ``hist_overflows``: samples at or beyond the histogram range,
* ``hids_window_drops``: failed HIDS submissions in this 128-event window,
* ``last_hids_err``: the most recent HIDS submission error in the window,
* ``rtfw_drops``: cumulative framework queue overflows since initialization,
* ``rtfw_max_depth``: cumulative queue high-water mark, or zero when
  ``CONFIG_RTFW_QUEUE_USAGE_STATS`` is disabled,
* ``rtfw_faults``: cumulative framework fault bits, and
* ``rtfw_status_err``: the result of reading the framework status snapshot.

The histogram has 256 buckets with an 8-us width and a 2048-us represented
range. Values from 2040 through 2047 us use the final bucket; values at or above
2048 us also saturate there and increment ``hist_overflows``. Statistics and
histogram contents are reset after every report.

The HID sample reserves 3072 bytes for ``rtfw_delivery`` through
``CONFIG_RTFW_WORKQ_STACK_SIZE``. This is 1536 bytes above the subsystem
default because the callback enters the Bluetooth HIDS path and formats a
64-bit latency report. The system workqueue retains its separate 2048-byte
stack for advertising work.

The reported value is a Zephyr and HIDS delivery benchmark, not the execution
time of the source ZLI.

Instrumentation
***************

Enable ``CONFIG_SAMPLE_RTFW_DEBUG_PIN`` to assert the board-specific debug
output around the direct source ISR. Measuring the input and debug output
together separates:

* edge-to-ZLI entry latency,
* source-ISR execution time, and
* the later edge-to-HIDS-submit metric printed by the sample.

``CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD`` adds a controlled NOP loop to the source
ISR. Use ``CONFIG_SAMPLE_RTFW_ISR_TEST_LOAD_CYCLES`` to select its size. This
option is for stress testing and should remain disabled for baseline
measurements.

The sample metadata builds base and instrumentation variants for both
supported boards.

Validation
**********

For a representative test:

1. Record the board, build configuration, controller version, connection
   parameters, and input stimulus.
2. Pair and establish an encrypted HID connection.
3. Generate repeatable input edges rather than relying only on manual button
   presses.
4. Capture several statistics windows and debug-pin traces.
5. Verify zero HIDS drops and histogram overflows under the intended load.
6. Repeat during unrelated Zephyr activity and radio traffic.
7. Compare against an equivalent Zephyr GPIO-callback implementation.
8. Verify disconnect, re-advertising, re-pairing, and settings persistence.

Measure the delivery stack high-water mark while exercising connection and
security setup, HIDS notifications, latency-report formatting, and an edge
burst. For example, make an instrumentation build with
``CONFIG_THREAD_ANALYZER=y``, ``CONFIG_THREAD_ANALYZER_AUTO=y``, and
``CONFIG_THREAD_ANALYZER_AUTO_INTERVAL=5``, then verify that
``rtfw_delivery`` retains an adequate margin for the intended workload.

Limitations
***********

The sample intentionally leaves debouncing, gestures, and application policy
outside the ZLI. A bouncing input therefore produces multiple HID events.
A sustained event storm can overflow the RTFW queue or delay HIDS submission;
both conditions are reported by the benchmark. The sample does not support
system PM and rejects ``CONFIG_PM`` at build time.
