.. _ug_nrf5340:

Working with nRF5340
####################

The |NCS| provides support for developing on the nRF5340 System on Chip (SoC) using the nRF5340 PDK (PCA10095).

Introduction
************

nRF5340 is a wireless ultra-low power multicore System on Chip (SoC) with two fully programmable Arm Cortex-M33 processors: a network core and an application core.
The |NCS| supports Bluetooth Low Energy communication on the nRF5340 SoC.

See the `nRF5340 Product Specification`_ for more information about the nRF5340 SoC.
:ref:`zephyr:nrf5340_dk_nrf5340` gives an overview of the nRF5340 PDK support in Zephyr.

Network core
============

The network core is an Arm Cortex-M33 processor with a reduced feature set, designed for ultra-low power operation.

This core is used for radio communication.
With regards to the nRF5340 samples, this means that the network core runs the radio stack and real-time procedures.
Currently, the following solutions are available for the network core:

* Bluetooth Low Energy Controller - compatible with several BLE samples.
  Both the BLE Controller from Zephyr and :ref:`nrfxlib:ble_controller` are supported.
* The :ref:`radio_test` sample that runs only on the network core and is used for testing the Radio peripheral.
  To start the network core, this sample requires any sample programmed on the application core.
  For example, you can use :ref:`zephyr:hello_world` for this purpose.

In general, this core should be used for real-time processing tasks involving low-level protocols and layers.

The board name for the network core in Zephyr is ``nrf5340_dk_nrf5340_cpunet``.

Application core
================

The application core is a full-featured Arm Cortex-M33 processor including DSP instructions and FPU.

Currently, the |NCS| provides the following solutions for the application core:

* high-level radio stack (the host part of the Bluetooth Low Energy stack) and application logic,
* samples running only on the application core (for example, NFC samples with nRF53 support).

In general, this core should be used for tasks that require high performance and application-level logic.

The board name for the application core in Zephyr is ``nrf5340_dk_nrf5340_cpuapp``.

The user application can run in the secure or non-secure domain.
Therefore, it can be built for two different board targets:

* ``nrf5340_dk_nrf5340_cpuapp`` for the secure domain,
* ``nrf5340_dk_nrf5340_cpuappns`` for the non-secure domain.

When built for the ``nrf5340_dk_nrf5340_cpuappns`` board, the :ref:`nrf9160_ug_secure_partition_manager` is automatically included in the build.

Inter-core communication
========================

Communication between the application core and the network core happens through a shared memory area.
The application core memory is mapped to the network core memory map.
This means that the network core can access and use the application core memory for shared memory communication.

Interprocessor Communication (IPC) is used to indicate to the other core that there is new data available to pick up.
The actual data exchange is handled by Open Asymmetric Multi Processing (OpenAMP).

Zephyr includes the `OpenAMP`_ library, which provides a complete solution for exchanging messages between the cores.
The IPC peripheral is presented to Zephyr as an Interprocessor Mailbox (IPM) device.
The OpenAMP library uses the IPM SHIM layer, which in turn uses the IPC driver in `nrfx`_.


Available samples
*****************

nRF5340 samples consist of two separate images: one that runs on the network core and one that runs on the application core.

Network samples
===============

The |NCS| provides the following samples for the nRF53 network core:

* :ref:`zephyr:bluetooth-hci-rpmsg-sample` - a Zephyr sample that implements a Bluetooth Low Energy controller.
  This sample must be programmed to the network core to run standard Bluetooth Low Energy samples on nRF5340.

  You might need to adjust the Kconfig configuration of this sample to make it compatible with the peer application.
  For example:

  * :option:`zephyr:CONFIG_BT_MAX_CONN` must be equal to the maximum number of connections supported by the application sample.
  * If the application sample uses a specific Bluetooth LE functionality, this functionality must be enabled in the network sample as well.
    For example, you must modify the configuration of the network sample to make it compatible with the :ref:`ble_throughput` sample::

      CONFIG_BT_CTLR_TX_BUFFER_SIZE=251
      CONFIG_BT_CTLR_DATA_LENGTH_MAX=251

    This configuration guarantees that the network sample can handle the Bluetooth LE DLE update procedure, which is used in the :ref:`ble_throughput` sample.

* :ref:`radio_test` - an sample application used for testing available modes of the Radio peripheral.

Application samples
===================

The |NCS| provides a series of :ref:`Bluetooth Low Energy samples <ble_samples>`, in addition to the :ref:`Bluetooth samples in Zephyr <zephyr:bluetooth-samples>`.
Most of these samples should run on the nRF5340 PDK, but not all have been thoroughly tested.
Samples that use non-standard features of the Bluetooth Low Energy controller, like the :ref:`ble_llpm` sample, are not supported.
Additionally, the |NCS| NFC samples are also available for nRF53 - they run only on the application core and do not require any firmware for the network core.

Some samples require configuration adjustments to the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample as described in the `Network samples`_ section.

These samples must be programmed to the application core, in the secure domain.


Building and programming a sample
*********************************

Depending on the sample, you must program only the application core (for example, when using NFC samples) or both the network and the application core.

.. note::
   On nRF53, the application core is responsible for starting the network core and connecting its GPIO pins.
   Therefore, to run any sample on nRF53, the application core must be programmed, even if the firmware is supposed to run only on the network core.
   You can use the :ref:`zephyr:hello_world` sample for this purpose.
   For details, see the code in :file:`zephyr/boards/arm/nrf5340_dk_nrf5340/nrf5340_cpunet_reset.c`.

Build and program both samples separately by following the instructions in :ref:`gs_programming_ses`.
Make sure to use ``nrf5340_dk_nrf5340_cpunet`` as board name when building the network sample, and ``nrf5340_dk_nrf5340_cpuapp`` when building the application sample.


Programming from the command line
=================================

To program a HEX file after building it with |SES|, open a command prompt in the build folder of the sample that you want to program and enter the following command::

    west flash

If you prefer to use nrfjprog (which is part of the `nRF Command Line Tools`_) instead, open a command prompt in the build folder of the network sample and enter the following commands to program the network sample::

    nrfjprog -f NRF53 --coprocessor CP_NETWORK --eraseall
    nrfjprog -f NRF53 --coprocessor CP_NETWORK --program zephyr/zephyr.hex

Then navigate to the build folder of the application sample and enter the following commands to program the application sample and reset the board::

    nrfjprog -f NRF53 --eraseall
    nrfjprog -f NRF53 --program zephyr/zephyr.hex

    nrfjprog --pinreset

Getting logging output
**********************

When connected to the computer, the nRF5340 PDK emulates three virtual COM ports.
In the default configuration, logging output of the application core sample is available on the third (last) COM port.
The first two COM ports are silent.

.. _logging_cpunet:

Logging output on the network core
==================================

In the default configuration, you cannot access the logging output of the network core sample.

To get logging output on the second COM port, you must connect certain pins on the nRF5340 PDK.
The following table lists which pins must be shorted:

.. list-table::
   :header-rows: 1

   * - 1st connection point
     - 2nd connection point
   * - 25
     - RxD
   * - 26
     - TxD
