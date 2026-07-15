.. _lib_rtfw:

App-domain real-time framework
##############################

.. contents::
   :local:
   :depth: 2

The app-domain real-time framework (RTFW) provides a narrow execution path for small operations that require execution latency independent of normal Zephyr thread scheduling.
It runs on the same application CPU and in the same application image as Zephyr.

Overview
********

RTFW combines a client-owned zero-latency interrupt (ZLI) with fixed-size communication mechanisms that allow Zephyr threads to control the fast path and receive events from it.

Zephyr provides application services such as scheduling, device drivers, power management, communication stacks, storage, and logging, but their execution latency can be affected by other system activity.

Some hardware interactions require a smaller and more predictable execution path, for example:

* Applying a short sequence of peripheral register operations
* Sampling an input at the point of interrupt entry
* Updating an output with bounded software overhead

RTFW addresses these interactions by dividing responsibilities between a small fast path and Zephyr:

* The fast path performs bounded hardware operations in a ZLI.
* Zephyr handles complex and potentially blocking operations.
* Fixed-size control and event channels connect the two execution contexts.

Only explicitly bounded work belongs in the fast path.
Bluetooth procedures, logging, storage, protocol processing, and application policy remain in Zephyr.

Choosing an integration approach
================================

Choose the simplest integration approach that meets the application's latency and hardware ownership requirements:

* Use standard Zephyr driver APIs or RTIO when they meet the latency requirements.
  These interfaces integrate with the Zephyr device model.
* Use a custom driver with Programmable Peripheral Interconnect (PPI) or Distributed Programmable Peripheral Interconnect (DPPI) when hardware event-to-task connections can implement the required response without CPU execution.
* Use RTFW when the response requires immediate, bounded CPU work in a ZLI while complex follow-up work remains in Zephyr.

When to use RTFW
================

Use RTFW when the following conditions apply:

* The required response cannot be implemented using hardware event-to-task connections alone
* The CPU must immediately perform a bounded operation
* Complex follow-up work can be deferred to Zephyr
* Latest-wins command handling and best-effort event delivery match the application

An RTFW source adapter is source-specific low-level code that can itself use PPI or DPPI.
RTFW standardizes Zephyr control, software-pended source entry, command status, event publication, deferred delivery, ownership, and overload diagnostics around that adapter.

Implementation
**************

The implementation centers on a client-owned source ZLI, a latest-wins command path, and optional event delivery to Zephyr.
The following terms describe the framework components and communication paths used throughout this page.

Framework core
   The reusable command mailbox, status tracking, event queue, doorbell, and delivery implementation in :file:`nrf/subsys/rtfw`.

Client
   The code that configures one RTFW instance and provides its callbacks.
   The current implementation supports one client per application image.

Source adapter
   The client code that owns and configures a specific peripheral, its direct interrupt, and its fast-path operations.
   The TIMER/GPIO and HID/GPIOTE implementations are examples of source adapters.

Source interrupt request (IRQ)
   The single client-owned direct interrupt used to enter the fast path.
   The associated interrupt service routine (ISR) can be entered when the peripheral asserts the IRQ or when the control plane software-pends it.

Fast path
   The bounded work executed from the source ZLI.
   It consists of framework command processing followed by the registered client fast-path handler.

Control plane
   The Zephyr-to-fast-path channel used to publish configuration or lifecycle commands.

Data plane
   The optional fast-path-to-Zephyr channel used to publish events without blocking the source ZLI.

Doorbell
   The Event Generator Unit (EGU) signal used internally by RTFW to request event delivery outside the ZLI.

Execution model
===============

The current framework instance has one client-owned source interrupt and four relevant execution contexts:

.. code-block:: text

   Zephyr thread
      |
      | rtfw_submit(command)
      v
   latest-wins command mailbox
      |
      | software-pend source IRQ
      v
   client-owned source ZLI
      |
      | rtfw_fastpath_run()
      |   1. process the newest pending command
      |   2. run the client fast-path handler
      |
      +----> fixed-size SPSC event queue
                 |
                 | EGU doorbell
                 v
          normal-priority EGU ISR
                 |
                 v
          RTFW delivery workqueue
                 |
                 v
          application event callback

The source ZLI is outside normal Zephyr scheduling, but it is not independent of the rest of the system.
It still shares the CPU, memory bus, peripherals, and interrupt controller.
Higher-priority interrupts can preempt it, and hardware or bus contention can still affect execution time.

Command processing
==================

The command path carries fixed-size state updates from Zephyr to the source ZLI and records the result of each processing attempt.

Command publication
-------------------

A Zephyr thread calls the :c:func:`rtfw_submit` function with a fixed-size command.
The framework copies it into the inactive mailbox slot, publishes a new token with release ordering, and calls the client's ``pend_source_irq`` callback.

The callback software-pends the same IRQ that the peripheral uses.
This is important because a configuration command must be processed even when the source peripheral is disabled and cannot generate a hardware event.

The mailbox has latest-wins semantics.
If multiple commands are published before the source ZLI processes them, the handler processes only the newest published command.
RTFW is therefore intended for desired-state updates, not for a sequence in which every intermediate command must execute.

Command handling
----------------

When the :c:func:`rtfw_fastpath_run` function starts, the framework compares the published and acknowledged tokens.
If a command is pending, the function does the following:

1. Copies the newest command to ZLI-local storage.
#. Invokes the registered command handler callback.
#. Records the command and handler result.
#. Acknowledges that processing attempt.
#. Attempts to enqueue a command-processed framework event.

The command handler must implement transactional behavior from the framework's perspective:

* Return zero only after the new state has been completely applied.
* On error, leave the previously applied state unchanged.
* Always finish in bounded time.

.. _lib_rtfw_command_status:

Command status
--------------

Command-processed events can be dropped when the event queue is full.
Use the command status to determine the processing result.

``requested``
   The newest command published by Zephyr.
   It describes the latest requested state, whether or not the fast path has processed it.

``pending``
   True when the newest published command has not yet been acknowledged by the fast path.

``attempted``
   The most recent command for which the command handler completed.
   An attempt can either succeed or fail.
   This field does not by itself mean that the operation failed.

``apply_result``
   The result returned by the handler for ``attempted``.
   Zero means success, while a negative errno value describes a rejected or failed attempt.

``applied``
   The most recent command applied successfully.
   A failed attempt does not replace this field.

RTFW initializes the status fields to zero before processing the first command.
These initial values do not indicate that command ID 0 was applied successfully.

For example, assume command ``A`` is already applied:

.. code-block:: text

   Initial state:
     requested=A, attempted=A, applied=A, pending=false, apply_result=0

   Zephyr publishes command B:
     requested=B, attempted=A, applied=A, pending=true

   The command handler rejects B with -EINVAL:
     requested=B, attempted=B, applied=A, pending=false,
     apply_result=-EINVAL

Event delivery
==============

The source fast path can call the :c:func:`rtfw_event_push` function to publish a fixed-size event.
The call uses a single-producer, single-consumer queue and never waits for Zephyr.
If the queue is full, the event is dropped and the framework records overflow diagnostics.

When delivery changes from idle to pending, RTFW triggers its EGU doorbell.
A normal-priority EGU ISR schedules the dedicated delivery workqueue.
The workqueue removes a bounded number of events per invocation and calls the application event callback if one is registered.

The application callback is outside the real-time path.
It can use kernel and application services, but slow or blocking work can delay later events and eventually overflow the fixed-size queue.
RTFW isolates the source ZLI from that delay.
It does not make the consumer service real-time.

Only the registered source fast-path execution context can produce events.
Publishing from a thread, the delivery callback, or another interrupt would violate the queue's single-producer contract.

Requirements
************

The library requires lock-free 32-bit atomic operations.
Additionally, symmetric multiprocessing (SMP) must be disabled with the :kconfig:option:`CONFIG_SMP` Kconfig option.
On Nordic hardware, the application devicetree must provide an ``rt-egu`` alias that references an EGU instance reserved for RTFW.
RTFW does not enforce hardware access, so the source adapter must reserve its source peripheral and IRQ and ensure that directly accessed pins or channels are not used elsewhere.

Configuration
*************

To use the RTFW library, enable the :kconfig:option:`CONFIG_RTFW` Kconfig option.

Additional Kconfig options
==========================

You can also configure the following options:

* :kconfig:option:`CONFIG_RTFW_COMMAND_DATA_SIZE` - Sets the maximum size of a command payload.
* :kconfig:option:`CONFIG_RTFW_EVENT_QUEUE_SIZE` - Sets the number of entries in the event queue.
  The value must be a power of two.
* :kconfig:option:`CONFIG_RTFW_QUEUE_USAGE_STATS` - Enables tracking of the maximum event queue depth.
* :kconfig:option:`CONFIG_RTFW_DRAIN_BUDGET` - Sets the maximum number of events processed during one delivery work item invocation.
* :kconfig:option:`CONFIG_RTFW_WORKQ_STACK_SIZE` - Sets the stack size of the delivery workqueue.
* :kconfig:option:`CONFIG_RTFW_WORKQ_PRIORITY` - Sets the priority of the delivery workqueue.
* :kconfig:option:`CONFIG_RTFW_EGU_IRQ_PRIORITY` - Sets the interrupt priority of the EGU doorbell.

Application integration
***********************

Integrating a source adapter requires selecting compatible interrupt priorities, implementing the source interrupt contract, and keeping all ZLI work within the fast-path restrictions.

.. _lib_rtfw_interrupt_priority:

Interrupt priority requirements
===============================

RTFW does not manage how multiple zero-latency interrupts are prioritized.
The source adapter must select its IRQ priority relative to every ZLI user on the CPU and keep its fast path bounded.
:kconfig:option:`CONFIG_ZERO_LATENCY_LEVELS` makes the configured number of highest interrupt priority levels available to ZLI IRQs.

The provided source adapters use the following coexistence policy:

* On nRF54L Series devices, two ZLI levels are configured.
  The Multiprotocol Service Layer (MPSL) uses priority ``0``, while RTFW uses priority ``1``, allowing MPSL to preempt RTFW.
* On the nRF54H20 SoC, the Bluetooth controller runs on a separate radio core.
  The application core uses one ZLI level and runs RTFW at priority ``0``.

Source interrupt contract
=========================

The framework does not register a generic ISR.
The source adapter provides an ISR that calls the :c:func:`rtfw_fastpath_run` function in each of the following cases:

* A peripheral event triggers the IRQ to service real-time source work.
* The :c:func:`rtfw_submit` function calls the ``pend_source_irq`` callback, which software-pends the IRQ so that the fast path can process a control command.
* Both causes are present during the same invocation.

After processing any pending command, the function always calls the registered fast-path callback.
The callback must therefore inspect the peripheral event state before treating the invocation as a hardware event.
A software-pended control invocation must not create a false source event.

The source adapter remains responsible for:

* Acknowledging peripheral events
* Keeping execution bounded
* Preserving hardware state across supported power states

Fast-path restrictions
======================

The application-provided command and fast-path callbacks execute in the source ZLI.
The application must ensure that these callbacks do not:

* Call Zephyr kernel APIs
* Block or sleep
* Allocate memory dynamically
* Log or format diagnostic output
* Perform cache maintenance
* Perform unbounded iteration
* Invoke complex protocol or driver stacks
* Access resources that can be powered down underneath the ZLI

The callbacks can use bounded hardware abstraction layer (HAL) register operations, local computation, and atomics that satisfy the platform contract.
They can also call the :c:func:`rtfw_event_push` function.

Samples using the library
*************************

The :ref:`rtfw_timer_gpio_sample` demonstrates the control and low-latency hardware response path.
A Zephyr shell publishes enable and period commands.
TIMER is the source IRQ, and the ZLI toggles an owned GPIO.

The :ref:`rtfw_hid_sample` demonstrates fast capture followed by deferred processing.
GPIOTE timestamps and normalizes an input edge in the source ZLI.
The event crosses the RTFW data plane, while Bluetooth Human Interface Device Service (HIDS) submission remains ordinary Zephyr work and is not part of the real-time guarantee.

Limitations
***********

The current memory-sharing contract is same-core ZLI-to-thread communication.
It does not provide a cross-core shared-memory transport.
On a multi-core SoC, RTFW remains on one application core.
Communication with another core uses a separate subsystem such as interprocessor communication (IPC).

The implementation is a singleton.
Supporting multiple clients, multiple producer IRQs, or cross-core producers would require a different ownership and queue model.

Dependencies
************

The RTFW library uses the following Zephyr components:

* Zephyr :ref:`kernel services <zephyr:kernel_api>`
* Zephyr :ref:`lock-free SPSC queue <zephyr:spsc_lockfree>`

The library also uses hardware abstraction layers from `nrfx`_.

.. _lib_rtfw_api:

API documentation
*****************

| Header file: :file:`include/rtfw/rtfw.h`
| Source files: :file:`subsys/rtfw/`

.. doxygengroup:: rtfw
