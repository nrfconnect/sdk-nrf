.. _ppi_seq:

PPI Sequencer
#############

.. contents::
   :local:
   :depth: 2

The PPI sequencer library uses hardware PPI connections to periodically trigger peripheral tasks and notify the application in batches, minimizing CPU wake-ups and reducing power consumption.

Overview
********

A typical use case for the PPI sequencer library is combining it with SPIM or TWIM to perform periodic transfers without requiring an interrupt after each transfer.
This makes it well suited for collecting sensor data that can be processed in batches.

Since the PPI connection triggers the task directly, only the resources needed by the hardware operation are activated.
The CPU and non-volatile memory (NVM) can remain idle, reducing both power consumption and CPU load.

By default, the PPI sequencer triggers a single task per period.
You can also configure it to trigger multiple tasks within a single period (for example, a pair of SPI transfers).
In that case, each additional task is defined with a timing offset measured from the start of the period.

The :ref:`ppi_seq_i2c_spi` helper module simplifies using the sequencer with the Zephyr device driver model and devicetree configuration.

Timing sources
**************

The PPI sequencer supports the following timing sources for controlling when tasks are triggered:

* GRTC with the interval feature - This source provides low power consumption and high precision.
  However, the nRF54 Series has only one channel that supports the interval feature, so only a single PPI sequencer instance can use it.
  It is not as low power as the RTC because it needs additional high-frequency clock activity to synchronize the 1 MHz and 32 kHz clocks used by the GRTC.

* RTC - If present, this source provides the lowest power consumption.
  Precision is limited by the 32 kHz tick.
  It supports multiple tasks in a single period.

* TIMER - This source provides the highest precision but has significantly higher power consumption during idle periods.
  It is recommended only for high-performance use cases where the PPI sequencer is used to reduce CPU load rather than power consumption.
  It does not support multiple tasks in a single period.

* GRTC with TIMER - You can use this mode when multiple tasks are triggered in each period.
  A GRTC compare event defines the main period, triggers the first task, and starts the TIMER.
  TIMER compares events then triggers the remaining tasks.
  The TIMER is stopped and cleared after all tasks have been triggered.

Batch notification
******************

The PPI sequencer supports the following methods for notifying the application when a batch of periods completes:

* System timer - A ``k_timer`` is scheduled to expire at the point in time when the requested number of operations completes.
  The user callback is called from the ``k_timer`` interrupt handler context.
  This method is low power and does not require additional hardware resources, but it should not be used for very short periods (hundreds of microseconds) because of the system timer precision.
  The timing source used by the PPI sequencer must be clocked from the same source as ``k_timer`` (the low-frequency clock), so using this notifier with a TIMER timing source is not recommended.

* TIMER peripheral in counter mode - This mode uses PPI to connect the ending event of the operation (for example, the SPIM ``END`` event) with a TIMER ``COUNT`` task.
  The TIMER counts the number of operations and triggers a ``COMPARE`` event when it reaches the requested count.
  This mode is much more precise because the batch interrupt fires immediately after the batch completes.
  However, the TIMER draws an additional 5–10 µA.
  It is recommended only for short periods where ``k_timer`` may be unreliable and the additional power consumption is negligible.

Power consumption
*****************

The power consumption gain compared to a straightforward software solution varies between the uses cases, but it is most significant for shorter periods.
As the period increases, the idle time has a larger impact on average current consumption, and the relative savings decrease.

* When using the RTC, you can expect about a 70% reduction in power consumption for periods shorter than 5 ms and about a 50% reduction for periods around 25 ms.
* When using the GRTC, you can expect about a 50% reduction in power consumption for periods shorter than 5 ms and about a 25% reduction for periods up to 25 ms.

Configuration
*************

A basic PPI sequencer configuration includes the following:

* Notifier structure
* Timing source
* Address of the task register to be triggered
* User callback

If your PPI sequencer is using multiple tasks, you must provide an array of task-address and offset pairs.
Configure the peripheral registers for all tasks used by the PPI sequencer before starting the sequencer.

.. _lib_ppi_seq_notifier_config:

Configuring the notifier
========================

The notifier structure must be persistent because the PPI sequencer references it at runtime.

System timer notifier
---------------------

Configuring the ``k_timer`` notifier may require providing an offset.
The offset is the number of microseconds that the last operation in the sequence takes, and it is used to calculate when the ``k_timer`` should expire.
If operations are short compared to the period, you can skip setting the offset because it does not significantly affect the calculation.

.. code-block:: c

    static struct ppi_seq_notifier notifier;

    notifier.type = PPI_SEQ_NOTIFIER_SYS_TIMER;
    notifier.offset = 0;

TIMER notifier
--------------

Configuring the TIMER notifier requires providing the address of the event used to count operations.
If there are multiple identical operations in a period, you must also pass that count to the notifier.
The notifier structure contains an ``nrfx_timer`` driver instance.
You must set the peripheral register address in that structure and register the interrupt handler.

.. code-block:: c

    static struct ppi_seq_notifier notifier;

    /* Register the interrupt. */
    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(timer20)),
        DT_IRQ_BY_IDX(DT_NODELABEL(timer20), 0, priority),
        nrfx_timer_irq_handler, &notifier.nrfx_timer.timer, 0);

    notifier.type = PPI_SEQ_NOTIFIER_NRFX_TIMER;
    notifier.nrfx_timer.timer.p_reg = (NRF_TIMER_Type *)DT_REG_ADDR(DT_NODELABEL(timer20));
    notifier.nrfx_timer.end_seq_event = nrfx_spim_end_event_address_get(&spim);
    notifier.nrfx_timer.extra_main_ops = 0;

.. _lib_ppi_seq_config:

Configuring the PPI sequencer
=============================

After configuring the notifier, initialize the PPI sequencer by populating a :c:struct:`ppi_seq_config` structure with the notifier, task address, timing source, and callback.
You can select an alternative timing source by setting ``.rtc_reg`` or ``.timer_reg`` in the configuration structure.
Make sure all peripheral registers for the tasks used by the PPI sequencer are configured before calling :c:func:`ppi_seq_init`.

Single task per period
----------------------

The following example configures the PPI sequencer with a single task per period using the default GRTC timing source:

.. code-block:: c

    static struct ppi_seq_notifier notifier;
    static struct ppi_seq seq;
    static const struct ppi_seq_config config = {
        .notifier = &notifier,
        .task = TASK_ADDR,
        .callback = ppi_seq_callback,
        /* Alternative timing source is selected by providing the peripheral register address. */
        /* .rtc_reg = NRF_RTCn or .timer_reg = NRF_TIMERn */
    };
    int rv;

    /* Notifier configuration. */
    ...

    rv = ppi_seq_init(&seq, &config);

Multiple tasks per period
-------------------------

The following example configures the PPI sequencer to trigger three tasks per period using GRTC with TIMER timing: ``TASK_ADDR0``, ``TASK_ADDR1`` (+20 µs), and ``TASK_ADDR2`` (+40 µs).
Each additional task is defined as a task-address and offset pair in the ``extra_ops`` array, where the offset is measured in microseconds from the start of the period.
The maximum number of extra tasks depends on the number of compare channels available in the TIMER or RTC peripheral.

.. code-block:: c

    static struct ppi_seq_notifier notifier;
    static struct ppi_seq seq;
    static const struct ppi_seq_extra_op ops[] = {
        {
            .task = TASK_ADDR1,
            .offset = 20,
        },
        {
            .task = TASK_ADDR2,
            .offset = 40,
        }
    };
    static const struct ppi_seq_config config = {
        .notifier = &notifier,
        .task = TASK_ADDR0,
        .callback = ppi_seq_callback,
        .extra_ops = ops,
        .extra_ops_count = ARRAY_SIZE(ops),
        .timer_reg = NRF_TIMERn,
        /* Alternative RTC timing source can be selected. */
        /* .rtc_reg = NRF_RTCn */
    };
    int rv;

    /* Notifier configuration. */
    ...

    rv = ppi_seq_init(&seq, &config);

Starting and stopping the sequencer
***********************************

Start a configured sequencer by calling :c:func:`ppi_seq_start` with the following parameters:

* Period in microseconds.
* Batch count, which specifies how many periods must complete before the user callback is called.
* Number of batches to execute before the sequencer stops automatically.
  Setting this to ``UINT32_MAX`` keeps the sequencer running until you stop it manually.

Stop the sequencer at any time by calling :c:func:`ppi_seq_stop`.

Limitations
***********

The following sections describe the known limitations of the PPI sequencer.

Time-critical operations
========================

While the sequencer is running, transfers are triggered at precise hardware-controlled intervals.
Any processing that must complete before the next batch starts must meet the timing deadline.
Failing to do so may go unnoticed and can lead to undefined behavior.
Take special care when using very short intervals or when the application performs long blocking operations such as flash writes.

System timer notifier precision
===============================

When the system timer is used as the batch completion notifier, it is scheduled to expire at an estimated time.
The estimation adds a quarter of the period to the expected completion time to ensure the timer fires after the batch completes.
Because of system timer jitter, this method is not recommended for short intervals.
For short intervals, use a TIMER in counter mode instead because the additional power consumption is negligible at those rates.

System timer clock source
=========================

Because the system timer timeout is estimated, the timing source used by the PPI sequencer must be clocked from the same source as the system clock.
The system clock uses the GRTC, which runs from the low-frequency clock.
Using a TIMER timing source together with the system timer notifier is therefore not recommended.

Dependencies
************

The module uses the following ``nrfx`` libraries and drivers:

* ``nrfx_gppi``
* ``nrfx_grtc``
* ``nrfx_timer``
* ``nrf_rtc``

API documentation
*****************

| Header file: :file:`include/ppi_seq/ppi_seq.h`
| Source files: :file:`lib/ppi_seq/`

.. doxygengroup:: ppi_seq
