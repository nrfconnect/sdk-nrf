.. _rt_framework_sample:

App-domain RT framework
#######################

.. contents::
   :local:
   :depth: 2

This sample is a minimal proof-of-concept of an *app-domain real-time framework*:
a small timing-critical component that executes outside the normal Zephyr scheduling
and synchronization mechanisms, owns a few hardware resources explicitly,
and is still controlled from the Zephyr-managed application through a minimal,
non-blocking control plane.

It demonstrates the simplest mechanism for an app-domain RT framework:

* a dedicated hardware timer owned by the RT component,
* a Zero-Latency Interrupt (ZLI) handler that toggles a GPIO,
* a control plane from the Zephyr side (enable/disable, configure period, read
  status/acknowledgment),
* an optional data plane (RT to Zephyr notification) over an EGU.

The RT component always runs as a ZLI on the **application core**, alongside the
Zephyr application, on a single core. On SoCs where the radio shares the
application core (nRF54L), a BLE peripheral runs concurrently as a coexistence
load. The RT-owned resources (timer, EGU, GPIOs) are selected from devicetree,
so moving to another SoC is a board-overlay change rather than a source change.

Requirements
************

The sample supports the following development kits:

.. table::
   :align: center

   +--------------------------+-----------------------------------+
   | Board                    | Build target                      |
   +==========================+===================================+
   | nRF54LM20 DK             | ``nrf54lm20dk/nrf54lm20b/cpuapp`` |
   +--------------------------+-----------------------------------+
   | nRF54H20 DK              | ``nrf54h20dk/nrf54h20/cpuapp``    |
   +--------------------------+-----------------------------------+

You also need a logic analyzer or an oscilloscope to observe the RT GPIO. On the
nRF54LM20 DK you can additionally use a BLE central (for example the
`nRF Connect for Mobile`_ app) to keep a connection up during the coexistence
test. On the nRF54H20 DK the radio runs on a separate core, so the BLE
coexistence load is not built (single application-core image).

Overview
********

The application is split into two domains that share only a small statically
allocated RAM region:

Zephyr application domain
   Runs ``main()``, the shell, the data-plane consumer, and (where built) the
   BLE peripheral. It only ever writes to the shared control block. It never
   touches the RT-owned peripherals.

RT component (fast path)
   The RT timer (the ``rt-timer`` devicetree alias) is driven directly through
   the nrfx HAL. Its interrupt is installed as a Zero-Latency Interrupt and
   toggles the ``rt-gpios`` pin. The ISR polls the control block once per tick
   and applies pending configuration.

.. note::
   The RT time base runs continuously after initialization, so the ZLI ISR
   always polls the control block. The ``enable`` flag only gates GPIO toggling.
   This keeps Zephyr completely away from the RT-owned peripherals: all
   peripheral access stays inside the RT component.

Resource ownership
*******************

The RT-owned resources are selected per board from devicetree (see the
overlays under ``boards/``): the ``rt-timer`` and ``rt-egu`` aliases and the
``rt-gpios`` / ``rt-debug-gpios`` properties of the ``/zephyr,user`` node. The
RT component takes only the register base and IRQ line from those nodes and
drives the peripherals through the nrfx HAL, so no Zephyr driver claims them:

* On **nRF54LM20** the timer/EGU instances (``TIMER20`` / ``EGU20``) are kept
  ``disabled``. ``TIMER10`` is reserved by MPSL and the RADIO shares the
  application core.
* On **nRF54H20** the application core owns global-domain instances
  (``timer130`` / ``egu130``), marked ``reserved`` so ownership and the IRQ are
  routed to the application core without instantiating a Zephyr driver. The
  radio/MPSL run on a separate core.

The shared RAM (``rt_ctrl`` / ``rt_evq``) is also owned by the RT component;
Zephyr only references it through ``rt_comm.h``.

Communication mechanism
***********************

All barriers and (optional) cache maintenance live in one place, ``rt_shared.h``,
so the rest of the framework never calls ``__DMB()`` or the cache API directly.

Control plane (Zephyr to RT)
   A single shared ``struct rt_control_block`` in RAM, published with a
   *seqlock*. Zephyr makes ``command_seq`` odd (write in progress), writes the
   command snapshot (``enable`` + ``period_ticks``), then makes ``command_seq``
   even again. The RT ISR only consumes a command observed with an even
   ``command_seq`` that differs from ``ack_seq``, reads the snapshot, and
   re-checks ``command_seq``; if it changed, it retries on the next tick. This
   guarantees the ISR never applies a half-written command, which a plain "write
   fields then bump a counter" scheme cannot, because the ZLI can preempt the
   writer at any instruction and ``irq_lock()`` does not mask it.

   The RT ISR owns the ``applied_enable`` / ``applied_period_ticks`` fields,
   which reflect what is actually programmed in hardware. Zephyr therefore
   reports both the *requested* and the *applied* state, plus a ``pending`` flag
   (``command_seq != ack_seq``), so a command in flight is never mistaken for
   one in effect. A command becomes effective within one RT period; the
   public ``rt_fw_*`` functions are mutex-serialized and may be called from
   multiple threads (but not from interrupt context).

Data plane (RT to Zephyr, optional)
   Enabled with :kconfig:option:`CONFIG_RT_FW_DATA_PLANE`. The RT ISR pushes
   events into a single-producer/single-consumer ring and triggers the RT EGU.
   The EGU interrupt runs at a normal Zephyr priority and schedules a work item
   that drains the ring, so the data is handled with ordinary Zephyr primitives
   outside the timing-critical path.

Cache and multi-core notes
   Because the RT component is a ZLI on the same core as the Zephyr threads, the
   shared data cache is self-coherent and ordering barriers alone are sufficient
   even on cached SoCs such as nRF54H20. Cache maintenance is only needed if a
   unidirectional buffer is additionally exposed to a DMA engine or another
   core. That case is opt-in through
   :kconfig:option:`CONFIG_RT_FW_SHARED_DMA_VISIBLE`, which aligns and pads the
   event entries to whole data cache line(s) and makes ``rt_shared.h`` perform
   explicit ``sys_cache_*`` flush (producer) and invalidate (consumer). This is
   the same cache-maintenance pattern Zephyr's DMM uses for DMA buffers.

   This option only handles cache *coherency*. Exposing the buffer to a real
   external master additionally requires placing it where that master can reach
   it, for example a devicetree ``zephyr,memory-region`` with
   ``ATTR_MPU_RAM_NOCACHE`` at a board-specific address, then linking ``rt_evq``
   into that region. The sample does not carve such a region because the PoC has
   no second bus master, and on nRF54H20 the application-core SRAM map is managed
   by the owned-memory/DMM framework. The cache maintenance runs inside the ZLI,
   so this option also adds to the RT ISR time budget and is outside the minimal
   BLE-coexistence fast path.

Configuration
*************

.. options:

* :kconfig:option:`CONFIG_RT_FW_MIN_PERIOD_US` / :kconfig:option:`CONFIG_RT_FW_MAX_PERIOD_US`
  - accepted range for ``rt freq``/``rt_fw_set_period_us()``. Values outside the
  range are rejected (not clamped). The maximum also bounds the worst-case
  control-plane latency, because the RT period is the polling interval. Defaults
  ``10`` / ``1000000``.
* :kconfig:option:`CONFIG_RT_FW_DATA_PLANE` - enable the RT to Zephyr data
  plane (RT EGU + event ring). Default ``n``.
* :kconfig:option:`CONFIG_RT_FW_EGU_IRQ_PRIO` - Zephyr (non-ZLI) interrupt
  priority for the data-plane EGU. Default ``5``.
* :kconfig:option:`CONFIG_RT_FW_SHARED_DMA_VISIBLE` - opt-in cache maintenance
  for the case where the event payload is also observed by a DMA engine or
  another core. Aligns/pads the entries to the cache line and adds
  ``sys_cache_*`` flush/invalidate. Leave ``n`` for the default app-core
  ISR/thread model. Default ``n``.
* :kconfig:option:`CONFIG_RT_FW_ISR_TEST_LOAD` - inject a controlled busy-loop
  into the RT ISR. Use it to study the impact on BLE timing and to find the RT
  ISR time budget. Default ``n``.
* :kconfig:option:`CONFIG_RT_FW_ISR_TEST_LOAD_CYCLES` - number of NOP
  iterations for the test load. Default ``100``.
* :kconfig:option:`CONFIG_RT_FW_DEBUG_PIN` - drive ``debug GPIO`` high for the
  duration of the RT ISR to measure ISR execution time directly. Default
  ``n``.

Building and running
********************

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20b/cpuapp nrf/samples/rt_framework
   west flash

Connect a terminal to the DK UART (115200 8N1). The shell command ``rt`` is the
control plane:

.. code-block:: console

   rt status            # show applied vs requested state, pending, seq/ack, telemetry
   rt freq 1000         # request the RT toggle period 1000 us (1 ms), in range
   rt start             # enable GPIO toggling
   rt stop              # disable GPIO toggling

The ``rt status`` output separates the *applied* state (owned by the RT ISR and
in effect in hardware) from the *requested* state (what Zephyr last asked for),
and shows ``pending`` while a request is not yet acknowledged.

To enable the data plane:

.. code-block:: console

   west build -b nrf54lm20dk/nrf54lm20b/cpuapp nrf/samples/rt_framework -- -DCONFIG_RT_FW_DATA_PLANE=y

To build for the nRF54H20 DK (single application-core image, no BLE load):

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp --sysbuild nrf/samples/rt_framework

Validation
**********

1. Probe the ``rt-gpios`` pin with a logic analyzer (on the nRF54LM20 DK this is
   the free connector pin **P1.04**, deliberately not a DK LED, so the RT-owned
   output never overlaps a Zephyr-driven pin). ``rt freq <us>`` followed by
   ``rt start`` should produce a square wave whose half-period equals the
   configured value. ``rt status`` shows ``ack_seq`` catching up to
   ``command_seq`` (``pending`` clearing and ``applied`` matching ``requested``),
   proving the control-plane handshake.
2. On the nRF54LM20 DK, keep a BLE central connected while the RT path runs. The
   link should stay up and no MPSL assert should fire. This is the coexistence
   evidence.
3. Build with :kconfig:option:`CONFIG_RT_FW_DEBUG_PIN` to measure the ISR
   duration on the ``rt-debug-gpios`` pin.
4. Increase :kconfig:option:`CONFIG_RT_FW_ISR_TEST_LOAD_CYCLES` until BLE
   starts to degrade to establish the RT ISR time budget.

Known constraints
******************

* The RT ISR priority assumptions are per SoC. On nRF54L, the RT ISR and the
  MPSL radio ISRs share the same NVIC priority (the single ZLI level) on the
  application core. They do not preempt each other, so a long RT ISR directly
  adds latency and jitter to the radio. Keep the RT ISR minimal. On nRF54H20 the
  radio runs on a separate core and does not share the application-core ZLI.
* The control plane must remain non-blocking, because the RT ISR runs above
  the level masked by ``irq_lock()``.
* The RT component always runs on the application core. Running it on another
  core/coprocessor is an explicit non-goal of this pattern.
* This sample validates the execution-split and control-plane mechanism with a
  timer/GPIO demonstrator.
