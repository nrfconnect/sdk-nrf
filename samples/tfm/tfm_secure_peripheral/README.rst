.. _tfm_secure_peripheral_partition:

.. ncs-sample::
   :title: TF-M secure peripheral

   The TF-M secure peripheral sample demonstrates the configuration and usage of secure peripherals in a Trusted Firmware-M (TF-M) partition.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Optionally, you can also use a logic analyzer.

Overview
********

A TF-M partition is a partition in the TF-M secure image.
It is used to implement the secure services and store secure data, and is isolated from the non-secure application code.
It exposes a number of functions or secure services to other partitions or to the firmware in the :ref:`Non-Secure Processing Environment (NSPE) <app_boards_spe_nspe>` (or both).
For more information on TF-M partitions, see :ref:`ug_tfm_partitioning`.

The sample demonstrates how to configure peripherals as secure peripherals and use them in a TF-M partition.
In this way, the peripheral is only accessible from the :ref:`Secure Processing Environment (SPE) <app_boards_spe_nspe>`, or from a TF-M partition with :ref:`isolation level 2 or higher<ug_tfm_architecture_isolation_lvls>`.

The TF-M partition used in this sample is located in the :file:`secure_peripheral_partition` directory at the sample root.
It contains the partition sources, build files and build configuration files.
The partition is built by the :ref:`TF-M build system <tfm_build_system>`, which is called by the |NCS| build system.

Configuration
*************

|config|

Configuration files
===================

The partition configuration and source files are in the :file:`secure_peripheral_partition` directory.

The following files are available:

* :file:`tfm_secure_peripheral_partition.yaml.in` - Partition manifest template for services, MMIO regions, IRQs, and dependencies.
* :file:`tfm_manifest_list.yaml.in` - Registers the partition with the TF-M manifest list.
* :file:`CMakeLists.txt` - TF-M partition build configuration.
* :file:`secure_peripheral_partition.c` - Partition implementation and IRQ handlers.
* :file:`util.c` and :file:`util.h` - Hex-encoding helpers for SPI output.

Configuring a secure peripheral
===============================

To configure a peripheral as secure:

* Enable and assign the peripheral to the TF-M partition.
* Define interrupt signals.

See the following sections for more details.

Enabling secure peripheral
--------------------------

To start using a peripheral as a secure peripheral, complete the following steps:

1. In your application's :file:`prj.conf` file, enable the peripheral for use in the SPE by setting the relevant Kconfig options to ``y`` (the following example assigns the TIMER1 peripheral as secure):

   .. code-block:: none

      CONFIG_NRF_TIMER1_SECURE=y

#. If you want to use GPIO pins or DPPI channels with secure peripherals and override the automatic setting by devicetree, assign them as secure pins or channels.
   You can do this with a bitmask.
   For example, the following setting assigns GPIO pin 23 as secure:

   .. code-block:: none

      CONFIG_NRF_GPIO0_PIN_MASK_SECURE=0x00800000

   .. note::
      If the secure peripheral uses GPIO pins, explicitly mark those pins as secure in the :file:`prj.conf` file.
      The GPIO controller is used to implement security by separation on Nordic devices: each pin is non-secure by default.
      Assigning a peripheral to the SPE (for example, with ``CONFIG_NRF_TIMER1_SECURE=y``) does not automatically secure its GPIO pins.
      TF-M only derives secure GPIO pins from devicetree for secure UART and SPIM instances.
      See :ref:`ug_tfm_building_secure_peripheral_gpio` for more details.

#. Assign the peripheral to the specific partition in the partition manifest YAML file :file:`tfm_secure_peripheral_partition.yaml.in`, one of the `Configuration files`_ provided with the sample.
   This step is required when the peripheral has security attributes that can be set by the user (such as split security attributes).
   Assigning the peripheral makes sure that the TF-M configures this peripheral as secure.

   For example, the following MMIO definition configures the TIMER1 peripheral as secure:

   .. code-block:: yaml

      "mmio_regions": [
              {
                      "name": "TFM_PERIPHERAL_TIMER1",
                      "permission": "READ-WRITE"
              },
      ]

   See `TF-M Secure Interrupt Integration`_ in the TF-M documentation for more details on the MMIO regions.

Integrating secure interrupt
----------------------------

If the secure peripheral generates interrupts, complete the following steps to integrate the interrupt with the TF-M interrupt mechanism:

1. Define the interrupt source and handling type in the partition manifest YAML file :file:`tfm_secure_peripheral_partition.yaml.in`, one of the `Configuration files`_ provided with the sample.

   For example, the following IRQ definition configures the TIMER1 peripheral as secure:


   .. code-block:: yaml

      "irqs": [
            {
                    "source": "TFM_TIMER1_IRQ",
                    "name": "TFM_TIMER1_IRQ",
                    "handling": "FLIH"
            },
      ]

   See `TF-M Secure Interrupt Integration`_  for more details regarding the secure interrupts.

#. If the type of interrupt handling is defined as the First Level Interrupt Handling (FLIH), define the FLIH handler.

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

   The return value specifies if the Second Level Interrupt Handling signal should be asserted or not.

#. If the type of interrupt handling is defined as the Second Level Interrupt Handling (SLIH), or if the FLIH handler specifies the signal to be raised, the interrupt is sent to the partition as a signal, which needs to be cleared.
   How to clear an interrupt signal depends on the type of interrupt handling.
   For example, the following code block shows how to reset a FLIH signal from TIMER1 and a SLIH signal for SPIM3:

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
   The sample demonstrates a workaround for this limitation by triggering an ``EGU`` interrupt in the firmware in the NSPE, which calls the secure partition to process the interrupt signals.

Configuring access to other TF-M partitions
===========================================

You can configure the secure peripheral partition to access services from other TF-M partitions (such as Crypto or Protected Storage).

For this, you must explicitly list these dependencies in the partition manifest YAML file :file:`tfm_secure_peripheral_partition.yaml.in`, one of the `Configuration files`_ provided with the sample.
For example, to allow access to the Protected Storage partition, add it to the ``dependencies`` section:

.. code-block:: yaml

   dependencies:
     - name: "TFM_SP_PS"

Building and Running
********************

.. |sample path| replace:: :file:`samples/tfm/tfm_secure_peripheral`

.. include:: /includes/build_and_run.txt

Testing
=======

The sample displays the following output in the console from the firmware in the NSPE:

.. code-block:: console

        SPP: send message: Success
        SPP: process signals: Success

The sample displays the following output in the console from the firmware in the SPE where ``N`` is the instance of the hardware used:

.. code-block:: console

        IRQ: GPIOTE_N count: 0x00000002
        IRQ: TIMER_N count: 0x00000001
        IRQ: GPIOTE_N count: 0x00000003
        IRQ: TIMER_N count: 0x00000002

In addition, the sample sends a hash as a text output using SPI on two GPIO pins.
Connect a logic analyzer to the pins that are used for the SPI output.

Dependencies
*************

This sample uses the TF-M module that can be found in the following location in the |NCS| folder structure:

* :file:`modules/tee/tfm/`
