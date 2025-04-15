.. _hpf_real_time_peripherals:

Real-time peripherals
#####################

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

.. contents::
   :local:
   :depth: 2

This document provides technical details and guidance on how to use the real-time (RT) peripherals integrated into the VPR CPU.
They are designed to enhance real-time performance and allow precise control over GPIO pads and other low-level hardware interactions.

Overview
********

The real-time peripherals consist of the following main components that interact to deliver cycle-accurate control and synchronization of GPIO pads and events:

* VPR TIMER (VTIM) - Provides precise timing control and synchronization.
* VPR IO (VIO) - A GPIO controller that can handle up to 16 pins.
* VPR Event Interface (VEVIF) - Enables the sourcing of interrupt requests (IRQs) from the VPR and interaction with the Distributed Programmable Peripheral Interconnect (DPPI).

The peripherals operate in real-time and are mapped to the processor's Control and Status Register (CSR) space.
They remain active even in wait, sleep, and deep sleep modes, making them ideal for energy-efficient designs.

Prerequisites
*************

Before you start using the RT peripherals, enable them by calling the :c:func:`nrf_vpr_csr_rtperiph_enable_set` function.

VPR TIMER (VTIM)
****************

The VPR TIMER (VTIM) offers the following features:

* Two 16-bit timers (counter 0 and counter 1) that can operate independently or combine to form a 32-bit timer.
* Precise synchronization with the VIO and VEVIF modules for generating event pulses upon timer completion.
* Operation at the same clock frequency as the CPU without a prescaler.

.. note::
   You can find the full API in the `nrfx API documentation`_.

TIMER modes
===========

Each timer can operate in several modes to control its counting behavior, which can be set using the :c:func:`nrf_vpr_csr_vtim_count_mode_set` function:

* :c:enumerator:`NRF_VPR_CSR_VTIM_COUNT_STOP` - Halts the timer when it reaches zero.
* :c:enumerator:`NRF_VPR_CSR_VTIM_COUNT_WRAP` - Restarts the timer from its maximum value (``0xFFFF``) after reaching zero.
* :c:enumerator:`NRF_VPR_CSR_VTIM_COUNT_RELOAD` - Reloads the timer with a pre-configured value when it reaches zero.
  The value is set using the :c:func:`nrf_vpr_cst_vtim_simple_counter_top_set` or :c:func:`nrf_vpr_cst_vtim_combined_counter_top_set` function.
* :c:enumerator:`NRF_VPR_CSR_VTIM_COUNT_TRIGGER_COMBINED` - Trigger (counter 0) or combined (counter 1) mode.

  * Trigger mode is set to counter 0.
    The counter stops at zero and will not immediately restart the countdown if there is a change in value in counter 0, or in the combined counter when counter 1 is set to combined mode.
    Instead, it restarts only after a VIO event pulse occurs or VEVIF task 0 is triggered.
  * Combined mode is set to counter 1.
    Counter 1 and counter 0 act as one, combined counter, with counter 1 covering the 16 most significant bits and counter 0 covering the 16 least significant bits.
    The mode for the combined counter is set in counter 0.
    In this mode, the system omits any counter event pulses that occur when counter 1 reaches zero.

Use the ``nrf_vpr_csr_vtim_simple_*`` API to set the counters in standalone mode.
You must specify the counter index:

* :c:func:`nrf_vpr_cst_vtim_simple_counter_get`
* :c:func:`nrf_vpr_cst_vtim_simple_counter_set`
* :c:func:`nrf_vpr_cst_vtim_simple_counter_top_get`
* :c:func:`nrf_vpr_cst_vtim_simple_counter_top_set`
* :c:func:`nrf_vpr_cst_vtim_simple_counter_add_set`
* :c:func:`nrf_vpr_cst_vtim_simple_counter_wait_set`

Use the following functions (``nrf_vpr_csr_vtim_combined_*`` ) to adjust the combined counter:

* :c:func:`nrf_vpr_cst_vtim_combined_counter_get`
* :c:func:`nrf_vpr_cst_vtim_combined_counter_set`
* :c:func:`nrf_vpr_cst_vtim_combined_counter_top_get`
* :c:func:`nrf_vpr_cst_vtim_combined_counter_top_set`
* :c:func:`nrf_vpr_cst_vtim_combined_counter_add_set`
* :c:func:`nrf_vpr_cst_vtim_combined_counter_wait_trigger`

Operational guidelines
======================

Familiarize yourself with the following guidelines for managing counters within the system:

* Writing any non-zero value to a counter starts the counter.
  You can do this using ``nrf_vpr_csr_vtim_{combined,simple}_counter_set``, which sets the counter to a given value, or ``nrf_vpr_csr_vtim_{combined,simple}_counter_add_set``, which adds a value to the counter's current value.
* Timers generate an event pulse when they reach zero, which may trigger actions in other RT peripherals.
* Timers stop when they reach zero or if you set the counter value to zero in the :c:enumerator:`NRF_VPR_CSR_VTIM_COUNT_STOP` mode.

Interrupt
=========

Counter 0 can generate an interrupt request if it is enabled using the :c:func:`nrf_vpr_csr_cnt_irq_enable_set` function.
The interrupt line triggered varies depending on the System on Chip (SoC):

.. list-table:: IRQ line triggered by counter 0
   :widths: auto
   :header-rows: 1

   * - Target
     - IRQ line

   * - nRF54L15 FLPR
     - 31

CPU stalling
============

CPU can be stalled until corresponding counter's event pulse is generated, using the :c:func:`nrf_vpr_csr_vtim_simple_wait_set` or :c:func:`nrf_vpr_csr_vtim_combined_wait_trigger` function.

VPR IO (VIO)
************

The VPR IO (VIO) offers the following features:

* 16 GPIO pins - Supports up to 16 pins, configurable as input or output.
* Buffered outputs - Allows high-speed parallel GPIO updates.
* Pin direction control - Allows each pin to be individually configured for input or output.

Mapping pins
============

VIO pin numbering differs from general pin numbering.
See the following table for pin mapping between GPIO and VIO for specific targets:

.. list-table:: Pin mapping between GPIO and VIO
   :widths: auto
   :header-rows: 1

   * - Target
     - VIO pins available
     - Corresponding GPIO pins

   * - nrf54L15 FLPR
     - 4,0,1,3,2,5..10
     - P2: 0..10

.. note::
   Routing the signal between VPR and physical pins may require SoC-specific configuration.
   For instance, on the nRF54L15 SoC, you must change the ownership of GPIO pin with ``nrf_gpio_pin_control_select(pin, NRF_GPIO_PIN_SEL_VPR);``.

Direction
=========

You can set the pin direction using the :c:func:`nrf_vpr_csr_vio_dir_set` function and toggle it with the :c:func:`nrf_vpr_csr_vio_dir_toggle_set` function.

Output
======

You can set the pin output values using the :c:func:`nrf_vpr_csr_vio_out_set` function and toggle it with the :c:func:`nrf_vpr_csr_vio_out_toggle_set` function.

Input
=====

You can access input pin values with the :c:func:`nrf_vpr_csr_vio_in_get` function.
To modify how frequently these values are updated, use :c:func:`nrf_vpr_csr_vio_mode_in_set`, which allows configuration in one of the following modes:

* :c:enumerator:`NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS` - The value is continuously updated with the current state of VIO pins, provided that the CPU is not in a sleep state.
* :c:enumerator:`NRF_VPR_CSR_VIO_MODE_IN_EVENT` - The value is updated only on a counter 1 event pulse, regardless of the CPU state.
* :c:enumerator:`NRF_VPR_CSR_VIO_MODE_IN_SHIFT` - Similarly to event mode, the value is updated on a counter 1 event pulse.
  Additionally, input is shifted.
  For more information, see the :ref:`hpf_real_time_input_shifting` documentation section.

When in continuous mode, VIO pin 0 generates an event pulse on any value change.
This pulse can request the VPR clock to start, even if the CPU is in sleep mode or the clock is turned off.

Buffering
=========

VIO supports queuing updates that are applied on the next timer event pulse, allowing synchronized GPIO updates.
You can access buffered analogues using the ``nrf_vpr_csr_vio_*_buffered_*`` functions.
Writing to buffers will set dirty bits, which you can check with ``nrf_vpr_csr_vio_*_buffered_dirty_check``.
A dirty buffer is transferred to its direct analogue on a counter 0 event pulse, clearing the corresponding dirty bit.
Writing to a dirty buffer will stall the CPU to prevent overruns.
Additionally, writing to a direct analogue will also write to the buffer, clearing the dirty bit (if set).
Do not use the :c:func:`nrf_vpr_csr_vio_in_buffered_get` function in continuous or event modes, as it will stall the CPU until the next counter 1 event pulse.

Shifting
========

VIO supports serialized input and output operations.

Output shifting
---------------

VIO supports serialized output operations, useful for reducing CPU load during high-frequency GPIO updates.
The shifting mode is configured using :c:struct:`nrf_vpr_csr_vio_mode_out_t`, where :c:member:`frame_width` defines frame width in bits.
You can use the following shifting modes:

* :c:enumerator:`NRF_VPR_CSR_VIO_SHIFT_NONE` - Shifting is disabled.
* :c:enumerator:`NRF_VPR_CSR_VIO_SHIFT_OUTB` - Shifting uses output and buffered output.
  This mode is optimized for wide parallel output streams, where output data needs to be loaded very frequently.
* :c:enumerator:`NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE` - VIO 0 is reserved for the clock output pin.
  Shifting uses output and buffered output.
  This mode is optimized for serial protocol master operations, such as SPI, where timing synchronization is critical.

You can set the advanced configuration for output shifting with the :c:func:`nrf_vpr_csr_vio_config_set` function.

.. note::
  For more information on output shifting, see the *Shifting modes and usage* section in the VPR peripheral description.

.. _hpf_real_time_input_shifting:

Input shifting
--------------

VIO supports serialized input operations through shifting, activated by setting :c:enumerator:`NRF_VPR_CSR_VIO_MODE_IN_SHIFT`.
Shifting process aligns with the serial clock and sampling point based on counter 1 event pulses.
This mode is intended to be used in conjunction with :c:enumerator:`NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE` output mode.

.. note::
  For more information on input shifting, see the *Input Shifting* section in the VPR peripheral description.

Combined access
===============

Since VIO is using 16 pins, some functionalities have been combined into a single register.
By using ``nrf_vpr_csr_vio_*_combined_*``, you can decrease the number of times registers are accessed, and consequently improve timings.

VPR Events Interface (VEVIF)
****************************

VEVIF is an event interface for VPR, allowing connection to the domain's DPPI system.
VEVIF can also generate IRQs to other CPUs.
Note that VEVIF is the only RT peripheral module with registers accessible through Advanced Peripheral Bus (APB).

Tasks
=====

VEVIF tasks are responsible for generating interrupts handled by the VPR interrupt controller.
You can trigger tasks in the following ways:

* By calling the :c:func:`nrf_vpr_task_trigger` function, usually from other core to trigger an IRQ on VPR.
* By calling the :c:func:`nrf_vpr_csr_tasks_set` function, which is done only locally from VPR to generate local IRQ.
* Through the DPPI system, configured with the :c:func:`nrf_vpr_csr_vevif_subscribe_set` function.

The interrupt service routine must clear the task bit using the :c:func:`nrf_vpr_csr_vevif_tasks_clear`.

Events
======

VEVIF events can generate pulses that trigger an IRQ line or DPPI channels.
You can trigger events through calling the :c:func:`nrf_vpr_csr_vevif_events_set` function (locally from VPR).
To source DPPI using VEVIF events, you must configure it with the :c:func:`nrf_vpr_csr_vevif_publish_set` function.
To source IRQ out of VPR using VEVIF events, you need to enable them using the :c:func:`nrf_vpr_int_enable` function (from remote side) or the :c:func:`nrf_vpr_csr_vevif_int_enable` function (from VPR).

Buffering
---------

Similarly to VIO, you can buffer VEVIF events using the :c:func:`nrf_vpr_csr_vevif_events_buffered_set` function.
Buffer overflows are avoided by stalling the CPU when writing to a dirty buffer.

Tasks and events availability
=============================

The availability of VEVIF tasks and events varies depending on the SoC:

.. list-table:: VEVIF availability on SoCs
   :widths: auto
   :header-rows: 1

   * - Resource
     - Availability
   * - VEVIF tasks
     - 16..22
   * - VEVIF tasks DPPI connections
     - 16..19 connected to DPPIC_00 channels 0..3
   * - VEVIF events
     - 16..22
   * - VEVIF events DPPI connections
     - 16..19 connected to DPPIC_00 channels 0..3

Use cases
*********

The following sections show how to use RT peripherals in selected scenarios.

Interaction between VTIM and VIO
================================

You can use VTIM to trigger GPIO updates:

* Timers generate event pulses upon reaching zero.
* VIO outputs are synchronized with these pulses, ensuring precise GPIO control.

Toggle VIO pin on VTIM event
----------------------------

In this example, VTIM (in reload combined mode) is set to generate an event every 64 million CPU cycles.
Whenever this event occurs, VIO is configured to toggle pin 9 (mapping to P2.09 - **LED0** on the nRF54L15 DK) using buffered output:

.. code-block:: c

   // Configure ownership of P2.09 pin (VIO pin 9) - nRF54L15 specific
   nrf_gpio_pin_control_select(NRF_GPIO_PIN_MAP(2, 9), NRF_GPIO_PIN_SEL_VPR);

   // Enable real-time peripherals
   nrf_vpr_csr_rtperiph_enable_set(true);

   // Configure counters to reload and combined mode
   nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
   nrf_vpr_csr_vtim_count_mode_set(1, NRF_VPR_CSR_VTIM_COUNT_TRIGGER_COMBINED);

   // Configure VIO pin 9 as output
   nrf_vpr_csr_vio_dir_set(1 << 9);

   // Configure reload value
   nrf_vpr_csr_vtim_combined_counter_top_set(64000000);

   // Start the timer
   nrf_vpr_csr_vtim_combined_counter_set(64000000);

   while (true)
   {
      // Configure buffer output to toggle pin 9
      nrf_vpr_csr_vio_out_toggle_buffered_set(1 << 9);

      // CPU is stalled until output buffer is clean.
   }
