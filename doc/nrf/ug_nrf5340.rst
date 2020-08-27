.. _ug_nrf5340:

Working with nRF5340
####################

The |NCS| provides support for developing on the nRF5340 System on Chip (SoC) using the nRF5340 PDK (PCA10095).

Introduction
************

nRF5340 is a wireless ultra-low power multicore System on Chip (SoC) with two fully programmable Arm Cortex-M33 processors: a network core and an application core.
The |NCS| supports Bluetooth Low Energy and NFC communication on the nRF5340 SoC.

See the `nRF5340 Product Specification`_ for more information about the nRF5340 SoC.
:ref:`zephyr:nrf5340pdk_nrf5340` gives an overview of the nRF5340 PDK support in Zephyr.

Network core
============

The network core is an Arm Cortex-M33 processor with a reduced feature set, designed for ultra-low power operation.

This core is used for radio communication.
With regards to the nRF5340 samples, this means that the network core runs the radio stack and real-time procedures.

Currently, the |NCS| provides the following solutions for the network core:

* :ref:`ug_ble_controller` (both the SoftDevice Controller and the Zephyr Bluetooth LE Controller)
* Samples that directly use the radio peripheral

See `Network samples`_ for more information.

In general, this core should be used for real-time processing tasks involving low-level radio protocol layers.

The board name for the network core in Zephyr is ``nrf5340pdk_nrf5340_cpunet``.

Application core
================

The application core is a full-featured Arm Cortex-M33 processor including DSP instructions and FPU.

Currently, the |NCS| provides the following solutions for the application core:

* High-level radio stack (the host part of the Bluetooth Low Energy stack) and application logic
* Samples running only on the application core (for example, NFC samples with nRF5340 support)

See `Application samples`_ for more information.

In general, this core should be used for tasks that require high performance and for application-level logic.

The M33 TrustZone divides the application MCU into secure and non-secure domains.
When the MCU boots, it always starts executing from the secure area.
The secure bootloader chain starts the :ref:`secure_partition_manager` sample, which configures a part of memory and peripherals to be non-secure and then jumps to the main application located in the non-secure area.

In Zephyr, :ref:`zephyr:nrf5340pdk_nrf5340` is divided into two different build targets:

* ``nrf5340pdk_nrf5340_cpuapp`` for the secure domain
* ``nrf5340pdk_nrf5340_cpuappns`` for the non-secure domain

When built for the ``nrf5340pdk_nrf5340_cpuappns`` board, the :ref:`secure_partition_manager` sample is automatically included in the build.

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

.. |note| replace:: To upgrade firmware on the network core, perform the steps for FOTA upgrade described below, replacing :file:`app_update.bin` (file used when upgrading firmware on the application core) with :file:`net_core_app_update.bin`.
   In addition, ensure that :option:`CONFIG_PCD` is enabled for the MCUBoot child image.
   For more details, see :ref:`nc_bootloader`.


.. include:: ug_nrf52.rst
   :start-after: fota_upgrades_start
   :end-before: fota_upgrades_end


Available samples
*****************

nRF5340 samples usually consist of two separate images: one that runs on the network core and one that runs on the application core.

Most samples that support nRF5340 are not dedicated exclusively to this device, but can also be built for other chips that have the radio peripheral (for example, the nRF52 Series).
The following subsections give a general overview of what samples can run on nRF5340.

Network samples
===============

The nRF5340 network core supports samples that directly use the radio peripheral, for example, :ref:`radio_test`.

For Bluetooth Low Energy, the |NCS| provides the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample.
This Zephyr sample is designed specifically to enable the Bluetooth LE Controller functionality on a remote MCU (for example, the nRF5340 network core) using the `RPMsg`_ protocol as a transport for Bluetooth HCI.
The sample implements the RPMsg transport using the `OpenAMP`_ library to communicate with a Bluetooth Host stack that runs on a separate core (for example, the nRF5340 application core).

This sample must be programmed to the network core to run standard Bluetooth Low Energy samples on nRF5340.
You can choose whether to use the SoftDevice Controller or the Zephyr Bluetooth LE Controller for this sample.
See :ref:`ug_ble_controller` for more information.

You might need to adjust the Kconfig configuration of this sample to make it compatible with the peer application.
For example:

* :option:`CONFIG_BT_MAX_CONN` must be greater than or equal to the maximum number of connections configured for the Bluetooth Host in the application core firmware.
* If the application sample uses specific Bluetooth LE functionalities, these functionalities must be enabled in the network sample as well.
  For example, you must modify the configuration of the network sample to make it compatible with the :ref:`ble_throughput` sample::

    CONFIG_BT_CTLR_TX_BUFFER_SIZE=251
    CONFIG_BT_CTLR_DATA_LENGTH_MAX=251

  This configuration guarantees that the network sample can handle the Bluetooth LE DLE update procedure, which is used in the :ref:`ble_throughput` sample.


Application samples
===================

The |NCS| provides a series of :ref:`Bluetooth Low Energy samples <ble_samples>`, in addition to the :ref:`Bluetooth samples in Zephyr <zephyr:bluetooth-samples>`.
Most of these samples should run on the nRF5340 PDK, but not all have been thoroughly tested.

Some Bluetooth LE samples require configuration adjustments to the :ref:`zephyr:bluetooth-hci-rpmsg-sample` sample as described in the `Network samples`_ section.

Additionally, the |NCS| provides :ref:`NFC samples <nfc_samples>` that are available for nRF5340.
These samples run only on the application core and do not require any firmware for the network core.

When programming any of these samples to the application core, configure :option:`CONFIG_BOARD_ENABLE_CPUNET` to select whether the network core should be enabled.
When radio protocols (Bluetooth LE, IEEE 802.15.4) are used, this option is enabled by default.

.. _ug_nrf5340_building:

Building and programming a sample
*********************************

Depending on the sample, you must program only the application core (for example, when using NFC samples) or both the network and the application core.

.. note::
   On nRF5340, the application core is responsible for starting the network core and connecting its GPIO pins.
   Therefore, to run any sample on nRF5340, the application core must be programmed, even if the firmware is supposed to run only on the network core, and the firmware for the application core must set :option:`CONFIG_BOARD_ENABLE_CPUNET` to ``y``.
   You can use the :ref:`nrf5340_empty_app_core` sample for this purpose.
   For details, see the code in :file:`zephyr/boards/arm/nrf5340pdk_nrf5340/nrf5340_cpunet_reset.c`.

To program only the application core, follow the instructions in :ref:`gs_programming_ses` and use ``nrf5340pdk_nrf5340_cpuapp`` or ``nrf5340pdk_nrf5340_cpuappns`` as build target.

When programming both the application core and the network core, you can choose whether you want to build and program both images separately or combined as a :ref:`multi-image build <ug_multi_image>`.

In a multi-image build, the image for the application core is the parent image, and the image for the network core is treated as a child image in a separate domain.
For this to work, the network core image must be explicitly added as a child image to one of the application core images.
See :ref:`ug_multi_image_defining` for details.

The network sample :ref:`zephyr:bluetooth-hci-rpmsg-sample` is automatically added to all Bluetooth Low Energy samples in the |NCS|.
When :option:`CONFIG_BT_RPMSG_NRF53` is set to ``y`` (the default), the build system automatically includes the sample as a child image in the ``nrf5340_pdk_nrf5340_cpunet`` core.

If the image in the network core is not added to the application core build, you can build and program both samples separately by following the instructions in :ref:`gs_programming_ses`.
Make sure to use ``nrf5340pdk_nrf5340_cpunet`` as build target when building the network sample, and ``nrf5340pdk_nrf5340_cpuapp`` or ``nrf5340pdk_nrf5340_cpuappns`` when building the application sample.


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
   * - P0.25
     - RxD
   * - P0.26
     - TxD

If you use flow control, you must also connect the RTS and CTS line as described in the next table:

.. list-table::
   :header-rows: 1

   * - 1st connection point
     - 2nd connection point
   * - P0.10
     - RTS
   * - P0.12
     - CTS
