.. _secure_partition_manager:

Secure Partition Manager
########################

.. contents::
   :local:
   :depth: 2

The Secure Partition Manager sample provides a reference use of the System Protection Unit peripheral.
This firmware is required to set up an nRF device with Trusted Execution (|trusted_execution|) so that it can run user applications in the non-secure domain.

.. note::
   An alternative for using the SPM is Trusted Firmware-M (TF-M). See :ref:`ug_tfm`.

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

The SPM can provide access to secure services to the application firmware.
See the :ref:`lib_spm` library for information about the available services.
See the :ref:`secure_services` for example code for using the secure services.

Requirements for the application firmware
=========================================

* The application firmware must be located in the slot_ns flash partition.
  For more details, see the partition configuration file for the chosen board (e.g. `nrf9160dk_nrf9160_partition_conf.dts`_ for the nRF9160 DK).
  Note that if you build your application firmware with the |NCS|, this requirement is automatically fulfilled.

* The application firmware must be built as a non-secure firmware for the build target (e.g. nrf9160dk_nrf9160ns for the nRF9160 DK).

Automatic building of SPM
=========================

The sample is automatically built by the non-secure applications when the non-secure build target is used (e.g. nrf9160dk_nrf9160ns).
However, it is not a part of the non-secure application.

Instead of programming SPM and the non-secure application at the same time, you might want to program them individually.
To do this, disable the automatic building of SPM by setting the option ``CONFIG_SPM=n`` in the ``prj.conf`` file of the application.

If this results in a single-image build, the start address of the non-secure application will change.
The security attribution configuration for the flash will change when SPM is not built as a sub-image.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp, nrf9160dk_nrf9160

Building and running
********************

.. |sample path| replace:: :file:`samples/spm`

.. include:: /includes/build_and_run.txt

The sample is built as a secure firmware image for the nrf9160dk_nrf9160 and nrf5340dk_nrf5340 build targets.
See `Automatic building of SPM`_ if you want to program it independently from the non-secure application firmware.


Testing
=======

Program both the sample and your application firmware to the development kit.
After power-up, the sample starts your application firmware.

Observe that the application firmware operates as expected.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_spm`
