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
* Bluetooth Low Energy Controller - compatible with several BLE samples,
* An application running only on network core, used for testing the Radio peripheral; requires any
sample for application core - just to start the network MCU

In general, this core should be used for real-time processing tasks involving low-level protocols and layers.

The board name for the network core in Zephyr is ``nrf5340_dk_nrf5340_cpunet``.

Application core
================

The application core is a full-featured Arm Cortex-M33 processor including DSP instructions and FPU.

Currently, the |NCS| provides the following solutions for the application core:
* High-level radio stack (the host part of the Bluetooth Low Energy stack) and application logic,
* Samples running only on the application core: e.g. NFC samples for nRF53

In general, this core should be used for tasks that require high performance and application-level logic.

The board name for the application core in Zephyr is ``nrf5340_dk_nrf5340_cpuapp``.

.. note::
   The M33 TrustZone divides the application core into secure (``nrf5340_dk_nrf5340_cpuapp``) and non-secure (``nrf5340_dk_nrf5340_cpuappns``) domains.
   However, all nRF5340 samples currently run in the secure domain, so you should not use the ``nrf5340_dk_nrf5340_cpuappns`` board.

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

The nRF Connect SDK provides the following samples for the nRF53 network core:
* :ref:`zephyr:bluetooth-hci-rpmsg-sample` - a Zephyr sample that implements a Bluetooth Low Energy controller.
This sample must be programmed to the network core to run standard Bluetooth Low Energy samples on nRF5340.

  You might need to adjust the Kconfig configuration of this sample to make it compatible with the peer application.
  For example:

  * :option:`zephyr:CONFIG_BT_MAX_CONN` must be equal to the maximum number of connections supported by the application sample.
  * If the application sample uses specific Bluetooth LE functionality, this functionality must be enabled in the network sample as well.
    For example, you must modify the configuration of the network sample to make it compatible with the :ref:`ble_throughput` sample::

      CONFIG_BT_CTLR_TX_BUFFER_SIZE=251
      CONFIG_BT_CTLR_DATA_LENGTH_MAX=251

    This configuration guarantees that the network sample can handle the Bluetooth LE DLE update procedure, which is used in the :ref:`ble_throughput` sample.

* :ref:`radio_test` - an application used for testing available modes of the Radio peripheral

Application samples
===================

The |NCS| provides a series of :ref:`Bluetooth Low Energy samples <ble_samples>`, in addition to the :ref:`Bluetooth samples in Zephyr <zephyr:bluetooth-samples>`.
Most of these samples should run on the nRF5340 PDK, but not all have been thoroughly tested.
Samples that use non-standard features of the Bluetooth Low Energy controller, like the :ref:`ble_llpm` sample, are not supported.
Additionally, the |NCS| NFC samples are also available for nRF53 - they run only on the application core and don't require any firmware for the network core.

Some samples require configuration adjustments to the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample as described in the `Network samples`_ section.

These samples must be programmed to the application core, in the secure domain.


Building and programming a sample
*********************************

Depending on the chosen sample, you must program just the application core (e.g. when using NFC samples) or both the network and the application cores.

.. note::
  On nRF53 the application core is responsible for starting the network core and connecting its GPIO pins. Therefore, to run any sample on nRF53,
  the application core must be programmed (e.g. with the :ref:`zephyr:hello_world` sample) even if the firmware is supposed to run only on the network MCU.
  For details see the code in: ``zephyr/boards/arm/nrf5340_dk_nrf5340/nrf5340_cpunet_reset.c``.

Build and program both samples separately by following the instructions in :ref:`gs_programming_ses`.
Make sure to use ``nrf5340_dk_nrf5340_cpunet`` as board name when building the network sample, and ``nrf5340_dk_nrf5340_cpuapp`` when building the application sample.

.. important::
   When programming the samples from an old version of |SES|, you might get an error message stating that the target cannot be identified.
   In this case, you can either modify your |SES| installation and projects to add support for programming nRF5340, or program the generated HEX files from the command line instead.

   See the following sections for more information.


Adding support for programming nRF5340 in SES (versions earlier than v4.40a)
============================================================================

The older versions of the Nordic Edition of |SES| (older than v4.40a) does not include SEGGER J-Link version v6.54c.
However, this J-Link version is required to program nRF5340 devices.

To add support for programming nRF5340 in |SES|, complete the following steps:

1. Download and install the latest `J-Link Software and Documentation Pack`_ (v6.54c or later).
#. Copy the :file:`JLink_x64.dll` file from the J-Link installation directory into the ``bin`` folder of your |SES| (Nordic Edition) folder, replacing the existing file.
   On Windows, this file is by default located in ``C:/Program Files (x86)/SEGGER/JLink/JLink_x64.dll``.
   Note that the file might be named differently for other operating systems.
#. Restart |SES|.
#. Open an |NCS| project with the full path to ``boards/arm/nrf5340_dk_nrf5340`` in the Zephyr repository as board directory and either ``nrf5340_dk_nrf5340_cpunet`` or ``nrf5340_dk_nrf5340_cpuapp`` as board name.
#. Right-click on the project in the Project Explorer and select **Options**.
#. Navigate to **Debug** > **Debugger**.
#. Double-click the value for **Target Device** and select **nRF9160** from the list.
   nRF5340 is not included in the list yet, but selecting nRF9160 sets the required configuration.
#. Double-click the value for **Target Device** again and enter **nRF5340** in the search field.
   Click **OK** to use this value.

You can now build and program the sample for nRF5340.


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
