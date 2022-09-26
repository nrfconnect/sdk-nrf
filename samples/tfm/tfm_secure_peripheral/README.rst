.. _tfm_secure_peripheral_partition:

TF-M secure peripheral partition
################################

.. contents::
   :local:
   :depth: 2

The TF-M secure peripheral partition sample demonstrates the configuration and usage of secure peripherals in a Trusted Firmware-M (TF-M) partition.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Optionally a logic analyzer can be used.

Overview
********

A secure partition is an isolated module that resides in TF-M.
It exposes a number of functions or secure services to other partitions and/or to the firmware in Non-Secure Processing Environment (NSPE).
TF-M already contains standard partitions such as crypto, protected storage and firmware update, but you can also create your own partitions.

The sample demonstrates how to configure peripherals as secure peripherals and use them in the secure partition.
In this way, the peripheral is only accessible from Secure Processing Environment (SPE), or from the secure partition with isolation level 2 or higher.

The secure partition is located in the ``secure_peripheral_partition`` directory.
It contains the partition sources, build files and build configuration files.
The partition is built by the TF-M build system.
See :ref:`tfm_build_system` for more details.

For more information on how to add custom secure partitions, see `TF-M secure partition integration guide`_.

Configuration
*************

To configure a peripheral as secure, the peripheral must be enabled and assigned to the partition, and any interrupt signals must be defined.

|config|

Secure peripheral
=================

To start using a peripheral as a secure peripheral, it must first be enabled for use in SPE.

.. code-block:: none

	CONFIG_NRF_TIMER1_SECURE=y
        CONFIG_NRF_SPIM3_SECURE=y
        CONFIG_NRF_GPIOTE0_SECURE=y

To use GPIO pins or DPPI channels with secure peripherals, assign them as secure pins or channels.
This is done with a bitmask, for example to assign GPIO pin 23 as secure:

.. code-block:: none

	CONFIG_NRF_GPIO0_PIN_MASK_SECURE=0x00800000

In addition, the peripheral must be assigned to the specific partition in the partition manifest file:

.. code-block:: none

        "mmio_regions": [
                {
                        "name": "TFM_PERIPHERAL_TIMER1",
                        "permission": "READ-WRITE"
                },
        ]

Secure interrupt
================

If the secure peripheral generates interrupts, the interrupt needs to be integrated with TF-Ms interrupt mechanism.
The interrupt source and handling type needs to be defined in the partition manifest file:

.. code-block:: none

        "irqs": [
                {
                        "source": "TFM_TIMER1_IRQ",
                        "name": "TFM_TIMER1_IRQ",
                        "handling": "FLIH"
                },
        ]

If the type of interrupt handling is defined as the First Level Interrupt Handling (FLIH), then the FLIH handler needs to be defined.
The return value specifies if the Second Level Interrupt Handling signal should be asserted or not.

.. code-block:: c

        psa_flih_result_t tfm_timer1_irq_flih(void)
        {
                /* Application specific handling */

                if (condition) {
                        return PSA_FLIH_SIGNAL;
                } else {
                        return PSA_FLIH_NO_SIGNAL;
                }
        }

If the type of interrupt handling is defined as the Second Level Interrupt Handling (SLIH), or the FLIH handler specifies the signal to be raised, the interrupt is sent to the partition as a signal.
How to clear an interrupt signal depends on the type of interrupt handling.

.. code-block:: c

        if (signals & TFM_TIMER1_IRQ_SIGNAL) {
                /* Application specific handling */

                /* FLIH: Reset signal */
                psa_reset_signal(TFM_TIMER1_IRQ_SIGNAL);
        }

        if (signals & TFM_SPIM3_IRQ_SIGNAL) {
                /* Application specific handling */

                /* SLIH: End of Interrupt signal */
                psa_eoi(TFM_SPIM3_IRQ_SIGNAL);
        }

.. note::

        TF-M interrupt signals only assert the signal but do not schedule the partition to run.
        In cases where the interrupt signal is preempting the non-secure execution, the interrupt signal is not processed until the next time the partition is scheduled to run.
        The sample demonstrates a workaround for this limitation by triggering an EGU0 interrupt in the firmware in NSPE which calls the secure partition to process the interrupt signals.


Building and Running
********************

.. |sample path| replace:: :file:`samples/tfm/tfm_secure_peripheral`

.. include:: /includes/build_and_run.txt

Testing
=======

The sample displays the following output in the console from the firmware in NSPE:

.. code-block:: console

        SPP: send message: Success
        SPP: process signals: Success

The sample displays the following output in the console from the firmware in SPE:

.. code-block:: console

        IRQ: GPIOTE0 count: 0x00000002
        IRQ: TIMER1 count: 0x00000001
        IRQ: GPIOTE0 count: 0x00000003
        IRQ: TIMER1 count: 0x00000002

In addition, the sample sends a hash as a text output using SPI on two GPIO pins.
Connect a logic analyzer to the pins that are used for the SPI output.

Dependencies
*************

This sample uses the TF-M module that can be found in the following location in the |NCS| folder structure:

* :file:`modules/tee/tfm/`
