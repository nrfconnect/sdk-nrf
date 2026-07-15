.. _rtfw_concept:

App-domain real-time framework concept
######################################

The app-domain real-time framework (RTFW) provides a narrow execution path for
small operations whose latency must not depend on normal Zephyr thread
scheduling. It runs on the same application CPU and in the same application
image as Zephyr.

RTFW is not a second operating system. It combines a client-owned
zero-latency interrupt (ZLI) with bounded communication mechanisms that allow
Zephyr threads to control the fast path and receive events from it.

Problem statement
*****************

Zephyr provides scheduling, device drivers, power management, communication
stacks, storage, logging, and other application services. These facilities are
appropriate for most application logic, but their execution latency can be
affected by scheduling, critical sections, interrupt masking, and other system
activity.

Some hardware interactions require a smaller and more predictable execution
path. Examples include:

* applying a short sequence of peripheral register operations,
* sampling an input at the point of interrupt entry,
* updating an output with bounded software overhead.

Moving an entire application outside Zephyr would give up services that are
still needed by the product. RTFW instead separates one small fast path from
the rest of the application:

* the fast path owns selected hardware and executes in a ZLI,
* Zephyr continues to own complex and potentially blocking operations,
* fixed-size control and event channels connect both contexts.

The fast path is therefore suitable only for work that can be explicitly
bounded. Bluetooth procedures, logging, storage, protocol processing, and
application policy remain on the Zephyr side.

Choosing a hardware integration approach
****************************************

RTFW is one of several ways to integrate latency-sensitive hardware with a
Zephyr application. Start with the most portable mechanism that satisfies the
required timing, ownership, and processing model.

Zephyr driver APIs and RTIO
===========================

Standard Zephyr driver APIs should be the first choice for common peripheral
operations. RTIO additionally provides asynchronous submission, queueing, and
composition of I/O operations. These mechanisms integrate with the Zephyr
device model and provide reusable application interfaces, but their
abstraction and scheduling costs might be unsuitable for the shortest
response-time requirements.

Custom drivers and PPI or DPPI
==============================

A custom driver can use Nordic HAL or nrfx APIs and hardware mechanisms such
as PPI or DPPI. Hardware event-to-task connections are preferable when the
required reaction can be expressed without CPU execution: they avoid
scheduling and interrupt-entry overhead entirely.

Custom drivers can also install specialized interrupts, including direct or
zero-latency interrupts. They offer full control, but each implementation must
define its own ownership, Zephyr communication, lifecycle, error reporting,
and concurrency rules.

RTFW
====

RTFW is intended for the narrower case in which hardware event-to-task
connections are insufficient and the CPU must immediately execute a small,
bounded operation. It standardizes the surrounding integration: Zephyr
control, software-pended source entry, command status, non-blocking event
publication, deferred delivery, ownership, and overload diagnostics.

These approaches are not mutually exclusive. An RTFW source adapter is
source-specific low-level code and can itself use PPI or DPPI. Choose RTFW only
when the fast operation requires bounded CPU work in the source ZLI, complex
follow-up work can be deferred to Zephyr, and its latest-wins control and
best-effort event semantics match the application.

Execution model
***************

The current framework instance has one client-owned source interrupt and four
relevant execution contexts:

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

The source ZLI is outside normal Zephyr scheduling, but it is not independent
of the rest of the system. It still shares the CPU, memory bus, peripherals,
and interrupt controller. Higher-priority interrupts can preempt it, and
hardware or bus contention can still affect execution time.

Coexistence with other ZLI users
********************************

RTFW does not arbitrate between zero-latency interrupt users. The source
adapter selects its IRQ priority, and the NVIC applies the normal priority
ordering. ``CONFIG_ZERO_LATENCY_LEVELS`` makes the configured number of highest
interrupt priority levels available to ZLI IRQs.

On nRF54L, MPSL uses priority 0 for time-critical radio processing. The
provided source adapters configure two ZLI levels and run RTFW at priority 1.
Because a lower numerical value has higher urgency, MPSL can preempt the RTFW
source ZLI.

On nRF54H20, the Bluetooth controller runs on the radio core. The application
core therefore uses one ZLI level and runs the RTFW source IRQ at priority 0,
without competing with radio-controller interrupts on that core.

These settings are target-specific integration policies, not properties
enforced by the framework core. A source adapter for another target must
account for every ZLI user on its CPU, select an appropriate relative priority,
and keep its fast path bounded.

Terminology
***********

Framework core
   The reusable command mailbox, status tracking, event queue, doorbell, and
   delivery implementation in :file:`nrf/subsys/rtfw`.

Client
   The code that configures one RTFW instance and provides its callbacks.
   The current implementation supports one client per application image.

Source adapter
   The client code that owns and configures a specific peripheral, its direct
   interrupt, and its fast-path operations. The TIMER/GPIO and HID/GPIOTE
   implementations are examples of source adapters.

Source IRQ
   The single client-owned direct interrupt used to enter the fast path. It can
   be asserted by the peripheral or software-pended by the control plane.

Fast path
   The bounded work executed from the source ZLI. It consists of framework
   command processing followed by the registered client fast-path handler.

Control plane
   The Zephyr-to-fast-path channel used to publish configuration or lifecycle
   commands.

Data plane
   The optional fast-path-to-Zephyr channel used to publish events without
   blocking the source ZLI.

Doorbell
   The EGU signal used internally by RTFW to request event delivery outside
   the ZLI.

Control plane
*************

Publishing a command
====================

A Zephyr thread calls ``rtfw_submit()`` with a fixed-size command. The
framework copies it into the inactive mailbox slot, publishes a new token with
release ordering, and calls the client's ``pend_source_irq`` callback.

The callback software-pends the same IRQ that the peripheral uses. This is
important because a configuration command must be processed even when the
source peripheral is disabled and cannot generate a hardware event.

The mailbox has latest-wins semantics. If multiple commands are published
before the source ZLI processes them, the handler processes only the newest
published command. RTFW is therefore intended for desired-state updates, not
for a sequence in which every intermediate command must execute.

Processing a command
====================

On entry to ``rtfw_fastpath_run()``, the framework compares the published and
acknowledged tokens. If a command is pending, it:

1. copies the newest command to ZLI-local storage,
2. invokes the registered command handler,
3. records the command and handler result,
4. acknowledges that processing attempt,
5. generates a command-processed framework event.

The command handler must implement transactional behavior from the
framework's perspective:

* return zero only after the new state has been completely applied,
* on error, leave the previously applied state unchanged,
* always finish in bounded time.

Command status terminology
==========================

``requested``
   The newest command published by Zephyr. It describes the latest requested
   state, whether or not the fast path has processed it.

``pending``
   True when the newest published command has not yet been acknowledged by the
   fast path.

``attempted``
   The most recent command for which the command handler completed. An attempt
   can either succeed or fail. This field does not by itself mean that the
   operation failed.

``apply_result``
   The result returned by the handler for ``attempted``. Zero means success, a
   negative errno value describes a rejected or failed attempt.

``applied``
   The most recent command applied successfully. A failed attempt does not
   replace this field.

For example, assume command A is already applied:

.. code-block:: text

   Initial state:
     requested=A, attempted=A, applied=A, pending=false, apply_result=0

   Zephyr publishes command B:
     requested=B, attempted=A, applied=A, pending=true

   The command handler rejects B with -EINVAL:
     requested=B, attempted=B, applied=A, pending=false,
     apply_result=-EINVAL

This distinction lets the application determine all of the following:

* which state is currently requested,
* whether the fast path has processed that request,
* whether processing succeeded,
* which configuration is still active after a failure.

Source IRQ contract
*******************

The framework does not register a generic ISR and must not be called from
every direct interrupt in the system. The source adapter selects and owns one
specific peripheral IRQ.

Every entry to that source ISR calls ``rtfw_fastpath_run()``, regardless of why
the IRQ became pending:

* a peripheral event can enter the ISR to service real-time source work,
* ``rtfw_submit()`` can software-pend it to process a control command,
* both causes can be present during the same entry.

After processing any pending command, ``rtfw_fastpath_run()`` always calls the
registered fast-path handler. The handler must therefore inspect the
peripheral event state before treating the entry as a hardware event. A
software-pended control entry must not create a false source event.

The source adapter remains responsible for:

* configuring and exclusively owning the peripheral,
* selecting the source IRQ and its ZLI priority,
* registering the direct ISR,
* acknowledging peripheral events,
* keeping execution bounded,
* preserving hardware state across supported power states.

Fast-path restrictions
======================

The command handler and fast-path handler execute in the source ZLI. They must
not:

* call Zephyr kernel APIs,
* block or sleep,
* allocate memory dynamically,
* log or format diagnostic output,
* perform unbounded iteration,
* invoke complex protocol or driver stacks,
* access resources that can be powered down underneath the ZLI.

The handlers can use bounded HAL register operations, local computation,
atomics that satisfy the platform contract, and ``rtfw_event_push()``.

Data plane
**********

The source fast path can publish a fixed-size event with
``rtfw_event_push()``. The call uses a single-producer, single-consumer queue
and never waits for Zephyr. If the queue is full, the event is dropped and the
framework records overflow diagnostics.

When delivery changes from idle to pending, RTFW triggers its EGU doorbell. A
normal-priority EGU ISR schedules the dedicated delivery workqueue. The
workqueue removes a bounded number of events per invocation and calls the
application event callback.

The application callback is outside the real-time path. It can use kernel and
application services, but slow or blocking work can delay later events and
eventually overflow the fixed-size queue. RTFW isolates the source ZLI from
that delay, it does not make the consumer service real-time.

Only the registered source fast-path execution context can produce events.
Publishing from a thread, the delivery callback, or another interrupt would
violate the queue's single-producer contract.

Resource and ownership model
****************************

An RTFW source adapter bypasses normal Zephyr drivers for the hardware used in
the fast path. Its devicetree overlay must therefore prevent another driver
from claiming the same peripheral, interrupt, or pin.

The current memory-sharing contract is same-core ZLI-to-thread communication.
It does not provide a cross-core shared-memory transport. On a multi-core SoC,
RTFW remains on one application core, communication with another core uses a
separate subsystem such as IPC.

The implementation is a singleton and requires ``CONFIG_SMP=n``. Supporting
multiple clients, multiple producer IRQs, or cross-core producers would
require a different ownership and queue model.

Mapping to the samples
**********************

The :ref:`rtfw_timer_gpio_sample` demonstrates the control and fast-actuation
path. A Zephyr shell publishes enable and period commands. TIMER is the source
IRQ, and the ZLI toggles an owned GPIO.

The :ref:`rtfw_hid_sample` demonstrates fast capture followed by deferred
processing. GPIOTE timestamps and normalizes an input edge in the source ZLI.
The event crosses the RTFW data plane, while Bluetooth HIDS submission remains
ordinary Zephyr work and is not part of the real-time guarantee.
