.. _nrf54l_features:

Features of the nRF54L Series
#############################

.. contents::
   :local:
   :depth: 2

The nRF54L15 DK embeds an Arm® Cortex®-M33 processor with multiprotocol 2.4 GHz transceiver and supports Bluetooth® 5.4.

For additional information, see the following documentation:

* Zephyr page on the :ref:`zephyr:nrf54l15dk_nrf54l15`
* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.

VPR core
********

The nRF54L15 DK has VPR core named *fast lightweight peripheral processor* (FLPR).
It is designed to support the RISC-V instruction set, and features key enhancements that enable efficient handling of basic and complex operations, as well as streamlined instruction processing.
It can be used as either a standalone processor or as a helper core.

As a helper processor, FLPR specializes in managing tasks that require real-time attention or low power consumption, effectively boosting the performance of the main processor.
It is versatile, capable of operating independently or as an integrated peripheral, with accessible components for easy control and customization.

Trusted Firmware-M (TF-M)
*************************

Trusted Firmware-M provides a configurable set of software components to create a Trusted Execution Environment.
When you build your application with CMSE enabled, the TF-M is automatically included in the build.

For more information about the TF-M, see :ref:`ug_tfm`.
See also :ref:`tfm_hello_world` for a sample that demonstrates how to add TF-M to an application.

Supported protocols
*******************

The nRF54L15 DK supports Bluetooth® Low Energy (LE) including Bluetooth Mesh, proprietary protocols (including Enhanced ShockBurst), Matter, and Thread.

Amazon Sidewalk
===============

Amazon Sidewalk is a shared network designed to provide a stable and reliable connection to your device.
Ring and Echo device can act as a gateway, meaning they can share a portion of internet bandwidth providing the connection and services to Sidewalk end devices.
Amazon Sidewalk for the nRF Connect SDK is based on two variants, one using Bluetooth® LE (more suited for home applications) and the other one using sub-GHz with the Semtech radio transceiver (for applications requiring longer range).

To learn more about the Amazon Sidewalk solution, see the `Amazon Sidewalk documentation`_ page.

Bluetooth Low Energy
====================

The Bluetooth LE radio is designed for very low-power operation.
When you develop a Bluetooth LE application, you must use the Bluetooth software stack.
This stack is split into two core components: the Bluetooth Host and the Bluetooth LE Controller.
The :ref:`ug_ble_controller` user guide contains more information about the two available Bluetooth LE Controllers, and instructions for switching between them.

See the :ref:`zephyr:bluetooth` section of the Zephyr documentation for information on the Bluetooth Host and open source Bluetooth LE Controller.
The |NCS| contains :ref:`ble_samples` that can be run on the nRF54L15 DK device.
In addition, you can run the :ref:`zephyr:bluetooth-samples` that are included from Zephyr.

For available libraries, see :ref:`lib_bluetooth_services` (|NCS|) and :ref:`zephyr:bluetooth_api` (Zephyr).

Bluetooth Mesh
--------------

Bluetooth Mesh operates on Bluetooth Low Energy (LE), and is implemented according to Bluetooth Mesh Profile Specification v1.0.1 and Bluetooth Mesh Model Specification v1.0.1.
For the application core, the |NCS| provides several :ref:`bt_mesh_samples`.
In addition, you can find Bluetooth Mesh samples with :ref:`Bluetooth samples in Zephyr <zephyr:bluetooth-samples>`.

IEEE 802.15.4
=============

Implementation of the IEEE 802.15.4 MAC layer frame technology that enhances network efficiency and reliability through intelligent decoding of frame control fields, which manage types, addressing, and control flags.

For the application core, the |NCS| provides a series of samples for the :ref:`Thread <ug_thread>`, and :ref:`Matter <ug_matter>` protocols.

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

The nRF54L15 DK supports MCUboot as its bootloader.
The following features are supported:

  * Software and hardware-based :ref:`cryptography<ug_nrf54l_cryptography>`
  * Hardware key management
  * Single image pair for dual-bank Device Firmware Update (DFU) targeted at the CPU application (the ``nrf54l15dk/nrf54l15/cpuapp`` board target)
  * Configuring MCUboot as a first-stage bootloader

The second-stage bootloader functionality and the serial recovery mode are currently not supported.

Supported DFU protocols
=======================

The DFU process in the nRF54L15 DK uses the MCUmgr protocol.
It can be used for performing updates over Bluetooth® Low Energy (LE) and serial connections.

For instructions on testing, see :ref:`nrf54l_testing_dfu`.
