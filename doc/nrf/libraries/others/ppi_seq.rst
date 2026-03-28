.. _ppi_seq:

PPI Sequencer
#############

.. contents::
   :local:
   :depth: 2

The PPI sequencer library allows to periodically trigger peripheral tasks and get notification callback when batch of periods are completed.
A typical use case is to combine it with SPIM or TWIM to perform periodic transfers without interrupts after each transfer.
It is recommended to be used for collecting sensor data that can be processed in batches.
This approach reduce power consumption and CPU load.
Since PPI connection is triggering the task, system activates only the resources needed by the given hardware operation.
NVM memory and CPU can remain in idle.
By default, the PPI sequencer supports a single task that is triggered periodically.
It is also possible to configure the PPI sequencer to use multiple tasks in a period (for example, trigger pair of SPI transfers per period).
Complex configuration specifies additional tasks with timing offset after which they should be triggered.
Offset is measured from the start of the period.

The PPI sequencer supports the following timings sources:

1. GRTC using ``interval`` feature.
   This source allows to get low power and high precision.
   However, nRF54 Series has only one channel that supports that feature so only a single instance of the PPI sequencer can utilize it.
   It is not as low power as RTC as additional time of high frequency clock activity is needed to synchronize 1 MHz and 32 kHz clocks used by the GRTC.
#. RTC
   If present, it allows to get the lowest power consumption. Precision is limited due to 32 kHz tick.
   It supports a complex case with multiple tasks in the period.
#. TIMER
   It allows the highest precision but has significantly higher power consumption in idle periods.
   It is recommended only for high performance use cases where PPI sequencer is used to reduce CPU load and not power consumption.
   It does not support a complex case with multiple tasks in the period.
#. GRTC with TIMER
   This mode is used when multiple tasks are triggered in each period.
   GRTC compare event is used for the main period, it triggers the first task and starts the TIMER.
   TIMER compare events are used to trigger remaining tasks.
   TIMER is stopped and cleared when all tasks are triggered.

The PPI sequencer supports following methods of notification about the batch completion:

1. System timer
   ``k_timer`` expires at the point in time when requested number of operations is completed.
   User callback is called from ``k_timer`` interrupt handler context.
   This method is low power and does not require additional resources but due to system timer precision it should not be used for too short periods.
   It is not recommended if periods are in hundreds of microseconds range.
   This mode requires that timing source used by the PPI sequencer is clocked from the same source as ``k_timer`` (Low frequency clock).
   Therefore, it is then not recommended to using TIMER with this type of the notifier.
#. TIMER peripheral in counter mode
   This mode is using PPI to connect ending event of the operation (for example, SPIM END event) with TIMER COUNT task.
   TIMER is counting the number of operations and triggers COMPARE event when requested number of events occurs.
   This mode is much more precise as batch interrupt is triggered immediately after the completion of the batch.
   However, TIMER consumes additional minimal current (5-10 uA).
   It is recommended only for short periods where ``k_timer`` may fail and additional power consumption can be neglected.

:ref:`ppi_seq_i2c_spi` is a helper module that simplifies the use of the sequencer with Zephyr device driver model and the devicetree configuration.

Power consumption
*****************

Power consumption gain compared to the straightforward software solution will vary between the uses cases but it is the most significant for shorter periods.
That is because as period increases, the bigger impact on the average current consumption has the idle period.
When RTC is used then for periods shorter than 5 ms you can expect 70 % reduction in the power consumption and 50 % reduction for periods around 25 ms.
When GRTC is used then for periods shorter than 5 ms you can expect 50 % reduction in the power consumption and 25 % reduction for periods up to 25 ms.

Configuration
*************

The simple PPI sequencer configuration includes:

* Notifier structure
* Timing source
* Address of the task register to be triggered
* User callback

Additionally, if multiple tasks are used then an array of pair of task address and offset need to be provided.
Peripheral configuration of tasks used by the PPI sequencer must be performed by the user prior to starting the sequencer.

.. _lib_ppi_seq_notifier_config:

Configuring the notifier
========================

The notifier structure need to be persistent as it is used by the PPI sequencer.

Configuring the ``k_timer`` notifier might require providing the offset.
Offset is the number of microseconds that the last operation in the sequence takes and it is used to calculate when ``k_timer`` should expire to notify the batch completion.
If operations are short compared to the period then setting the offset may be skipped as it does not play a significant role in the calculation.

.. code-block:: c

    static struct ppi_seq_notifier notifier;

    notifier.type = PPI_SEQ_NOTIFIER_SYS_TIMER;
    notifier.offset = 0;

Configuring the TIMER notifier requires providing the address of the event that is used to count the operations.
If there are more the same operations in the period then that information also need to be given to the notifier.
TIMER notifier structure contains the instance of the ``nrfx_timer`` driver.
User need to configure the peripheral register address in that structure and register the interrupt handler.

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

Configuration of the PPI sequencer with a single task per period and GRTC (default) timing source.

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

Configuration of the PPI sequencer with multiple tasks and GRTC with TIMER timing requires additional configuration.
The following code configures the PPI sequencer to periodically trigger the following sequence: ``TASK_ADDR0``, ``TASK_ADDR1`` (+20 us), ``TASK_ADDR2`` (+40 us).
The maximum number of extra tasks depends on number of compare channels in the TIMER or RTC peripheral.

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

Usage
=====

Configured sequencer is started by calling :c:func:`ppi_seq_start` and specifying the following parameters:

* Period (in microseconds)
* Batch count which specifies after how many periods the user callback is called
* Number of times batches that shall be executed until the sequencer is stopped. ``UINT32_MAX`` indicates to run until stopped manually.

The sequencer can be manually stopped using :c:func:`ppi_seq_stop`.

Limitations
***********

Time critical operations
========================

When the sequencer is running then transfers are triggered simultaneously so any operations that need to be completed on the batch completion need to be performed on time.
Failing to meet the timing requirements may be unnoticed and may lead to an undefined behavior.
Special care need to taken in case of very short intervals or when the application may perform long blocking operations (for example flash operations).

System timer notifier
=====================

Short intervals
---------------

When the system timer is used as batch completion notifier then it is scheduled to expire at estimated time when a batch is completed.
Estimation adds quarter of the period to the expected time to ensure that it is triggered after a batch is completed.
Due to system timer jitter it is not recommended to use it for short intervals.
In case of short intervals power consumption is not that critical so it is recommended to use a TIMER in a counter mode.

Clock source
------------

Since system timer timeout is estimated, it is important to use the same clock source for the timing source as the one used for the system clock.
The system clock is using GRTC which is clocked from the LF clock source so it is not recommended to use the TIMER if system clock is used.
