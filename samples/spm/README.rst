.. _secure_partition_manager:

Secure Partition Manager
########################

.. contents::
   :local:
   :depth: 2

The Secure Partition Manager (SPM) sample provides a reference use of the System Protection Unit peripheral.
This firmware sets up an nRF device with Trusted Execution (|trusted_execution|) so that it can run user applications in the non-secure domain.

.. note::
   SPM is deprecated as of |NCS| v2.1.0 and will be removed in a future version of the SDK.
   :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` has replaced SPM as the trusted execution solution.

To use the Secure Partition Manager instead of TF-M, do the following:

* Disable the automatic inclusion of TF-M by setting the option :kconfig:option:`CONFIG_BUILD_WITH_TFM` to ``n`` in the project configuration.
* Set the option :kconfig:option:`CONFIG_SPM` to ``y``.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample uses the SPM to configure secure attributions and jump into the non-secure application.

The SPM utilizes the SPU peripheral to configure security attributions for flash, SRAM, and peripherals.
After the configuration setup is complete, the sample loads the application firmware that is located on the device.

Security attribution configuration
==================================

See the :ref:`lib_spm` subsystem for information about the security attribution configuration that is applied.

If your application requires a different security attribution configuration, you must update the SPM sample code to reflect this.

Secure Services
===============

The SPM can provide the application firmware with access to Secure Services.
See the :ref:`lib_spm` library for information about the available services.

Requirements for the application firmware
=========================================

* The application firmware must be located in the ``slot_ns`` flash partition.
  For more details, see the partition configuration file for the chosen board (for example, `nrf9160dk_nrf9160_partition_conf.dts`_ for the nRF9160 DK).
  If you build your application firmware with the |NCS|, this requirement is automatically fulfilled.

* The application firmware must be built as a non-secure firmware for the build target (for example, ``nrf9160dk_nrf9160_ns`` for the nRF9160 DK).

Automatic building of SPM when SPM is enabled
=============================================

The sample is automatically built by the non-secure applications when the non-secure build target (for example, ``nrf9160dk_nrf9160_ns``) is used and you configure it to include Secure Partition Manager.
However, it is not a part of the non-secure application.

Instead of programming SPM and the non-secure application at the same time, you might want to program them individually.
To do this, disable the automatic building of SPM by setting the option ``CONFIG_SPM=n`` in the :file:`prj.conf` file of the application.

If this results in a single-image build, the start address of the non-secure application will change.
The security attribution configuration for the flash will change when SPM is not built as a sub-image.

Building and running
********************

.. |sample path| replace:: :file:`samples/spm`

.. include:: /includes/build_and_run.txt

Testing
=======

Program both the sample and your application firmware to the development kit.
After power-up, the sample starts your application firmware.

Observe that the application firmware operates as expected.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_spm`
