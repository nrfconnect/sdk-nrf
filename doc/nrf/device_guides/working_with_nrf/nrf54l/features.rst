.. _nrf54l_features:

Features of the nRF54L15 PDK
############################

.. contents::
   :local:
   :depth: 2

The nRF54L15 PDK embeds an Arm® Cortex®-M33 processor with multiprotocol 2.4 GHz transceiver and supports Bluetooth® 5.4.

For additional information, see the following documentation:

* Zephyr page on the :ref:`zephyr:nrf54l15pdk_nrf54l15`
* :ref:`ug_nrf54l15_gs`
* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.

Supported protocols
*******************

The nRF54L15 PDK supports Bluetooth Low Energy (LE), proprietary protocols (including Enhanced ShockBurst), Matter, and Thread.

Amazon Sidewalk
===============

Amazon Sidewalk is a shared network designed to provide a stable and reliable connection to your device.
Ring and Echo device can act as a gateway, meaning they can share a portion of internet bandwidth providing the connection and services to Sidewalk end devices.
Amazon Sidewalk for the nRF Connect SDK is based on two variants, one using Bluetooth® LE (more suited for home applications) and the other one using sub-GHz with the Semtech radio transceiver (for applications requiring longer range).

To learn more about the Amazon Sidewalk solution, see the `Sidewalk documentation`_ page.

Bluetooth Low Energy
====================

The Bluetooth LE radio is designed for very low-power operation.
When you develop a Bluetooth LE application, you must use the Bluetooth software stack.
This stack is split into two core components: the Bluetooth Host and the Bluetooth LE Controller.
The :ref:`ug_ble_controller` user guide contains more information about the two available Bluetooth LE Controllers, and instructions for switching between them.

See the :ref:`zephyr:bluetooth` section of the Zephyr documentation for information on the Bluetooth Host and open source Bluetooth LE Controller.
The |NCS| contains :ref:`ble_samples` that can be run on the nRF54L15 PDK device.
In addition, you can run the :ref:`zephyr:bluetooth-samples` that are included from Zephyr.

For available libraries, see :ref:`lib_bluetooth_services` (|NCS|) and :ref:`zephyr:bluetooth_api` (Zephyr).

Enhanced ShockBurst
===================

.. include:: ../../../protocols/esb/index.rst
   :start-after: esb_intro_start
   :end-before: esb_intro_end

See the :ref:`ug_esb` user guide for information about how to work with Enhanced ShockBurst.
To start developing, check out the :ref:`esb_ptx` and :ref:`esb_prx` samples.

Matter
======

.. include:: ../../../protocols/matter/index.rst
   :start-after: matter_intro_start
   :end-before: matter_intro_end

Matter in the |NCS| supports the *System-on-Chip, multiprotocol* platform design for the nRF54L15 SoC using Matter over Thread.
You can read more about other available platform designs for Matter on the :ref:`Matter platform design page<ug_matter_overview_architecture_integration_designs>`.
For more information about the multiprotocol feature, see :ref:`ug_multiprotocol_support`.

See the :ref:`ug_matter` user guide for information about how to work with Matter applications.
To start developing, check the :ref:`matter_samples`.

Thread
======

.. include:: ../../../protocols/thread/index.rst
   :start-after: thread_intro_start
   :end-before: thread_intro_end

You can read more about other available platform designs for Thread on the :ref:`OpenThread architectures page<ug_thread_architectures>`.
For more information about the multiprotocol feature, see :ref:`ug_multiprotocol_support`.

To start developing, check out the :ref:`openthread_samples`.

Near Field Communication
========================

Near Field Communication (NFC) is a technology for wireless transfer of small amounts of data between two devices that are in close proximity.
The range of NFC is typically less than 10 cm.

Refer to the :ref:`ug_nfc` page for general information.
See the :ref:`nfc_samples` and :ref:`lib_nfc` for the samples and libraries that the |NCS| provides.

MCUboot bootloader support
**************************

The nRF54L15 PDK supports MCUboot as its bootloader, in the experimental phase.
This means the following:

  * Only software cryptography is supported.
  * Single image pair is supported for dual-bank Device Firmware Update (DFU) targeted at the CPU application (the ``nrf54l15pdk_nrf54l51_cpuapp`` build target).
  * MCUboot can be configured as a first-stage bootloader (second-stage bootloader functionality is not yet available).
  * Serial recovery mode is also not yet supported.

Supported DFU protocols
=======================

The DFU process in the nRF54L15 PDK uses the MCUmgr protocol.
It can be used for performing updates over Bluetooth® Low Energy (LE) and serial connections.

Testing the DFU solution
========================

You can evaluate the DFU functionality by running the :zephyr:code-sample:`smp-svr` sample for the ``nrf54l15pdk_nrf54l51_cpuapp`` build target, which is available for both Bluetooth LE and serial channels.
This allows you to build and test the DFU solutions that are facilitated through integration with child images and the partition manager.

To compile the SMP server sample for testing secondary image slots on external SPI NOR flash, run the following command:

.. code-block:: console

   west build -b nrf54l15pdk_nrf54l15_cpuapp -d build/smp_svr_54l_3 zephyr/samples/subsys/mgmt/mcumgr/smp_svr -T sample.mcumgr.smp_svr.bt.nrf54l15pdk.ext_flash

.. note::

   Make sure to use the correct build target depending on your PDK version:

   * For the PDK revision v0.2.1, AB0-ES7, use the ``nrf54l15pdk_nrf54l15_cpuapp`` build target.
   * For the PDK revisions v0.3.0 and v0.7.0, use the ``nrf54l15pdk_nrf54l15_cpuapp@0.3.0`` build target.

This configuration sets up the secondary image slot on the serial flash memory installed on the nRF54L15 PDK.
It also enables the relevant SPI and the SPI NOR flash drivers.
