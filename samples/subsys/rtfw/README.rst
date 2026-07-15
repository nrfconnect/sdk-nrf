.. _rtfw_samples:

Event-driven app-domain real-time framework
###########################################

.. toctree::
   :maxdepth: 1
   :hidden:

   CONCEPT
   timer_gpio/README
   hid/README

The app-domain RTFW runs small, bounded hardware operations in a
zero-latency interrupt on the same application CPU as Zephyr. Complex services
remain in Zephyr and communicate with the fast path through fixed-size command
and event channels.

The implementation is in :file:`nrf/subsys/rtfw`, and the public API is in
:file:`nrf/include/rtfw/rtfw.h`.

Concept
*******

Read :ref:`rtfw_concept` before integrating a source adapter. It introduces the
problem addressed by RTFW, terminology, execution model, communication
channels, source IRQ contract, and limitations.

Samples
*******

Two samples exercise the same subsystem and public API:

* :ref:`rtfw_timer_gpio_sample` provides shell-controlled TIMER/GPIO actuation.
* :ref:`rtfw_hid_sample` delivers GPIOTE input edges to Bluetooth HIDS.

Their documentation contains board resource maps, build and flash commands,
runtime instructions, configuration variants, expected output, benchmarks,
and source-specific limitations.

Supported boards
****************

The supplied source adapters support nRF54LM20 DK and nRF54H20 DK. Their board
overlays and target-specific constraints are documented by each sample.

Building
********

See :ref:`rtfw_timer_gpio_sample` or :ref:`rtfw_hid_sample` for target-specific
build, sysbuild, flash, and configuration-variant commands.

Validation
**********

Unit tests
==========

The :file:`nrf/tests/subsys/rtfw` native test covers:

* command publication, coalescing, status, failures, and token wrap;
* event ordering, overflow accounting, and bounded delivery;
* deterministic mailbox, SPSC, and doorbell interleavings;
* public API validation and delivery without an event handler.

Run it with:

.. code-block:: console

   ./zephyr/scripts/twister -p native_sim -T nrf/tests/subsys/rtfw
