.. _secure_partition_manager:

nRF9160: Secure Partition Manager
#################################

The Secure Partition Manager sample provides a reference use of the System Protection Unit peripheral.
This firmware is required to set up the nRF9160 DK so that it can run user applications in the non-secure domain.

Overview
********

The sample uses the SPM to configure secure attributions for the nRF9160 SiP and jump into the non-secure application.

The SPM utilizes the SPU peripheral to configure security attributions for the nRF9160 flash, SRAM, and peripherals.
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
  For more details, see the `nrf9160_pca10090_partition_conf.dts`_ file in the nrf9160_pca10090 board definition.
  Note that if you build your application firmware with the |NCS|, this requirement is automatically fulfilled.

* The application firmware must be built as a non-secure firmware for the nrf9160_pca10090ns board.

Automatic building of SPM
=========================

The sample is automatically built by the non-secure applications when the nrf9160_pca10090ns board is used.
However, it is not a part of the non-secure application.

Instead of programming SPM and the non-secure application at the same time, you might want to program them individually.
To do this, disable the automatic building of SPM by setting the option ``CONFIG_SPM=n`` in the ``prj.conf`` file of the application.

If this results in a single-image build, the start address of the non-secure application will change.
The security attribution configuration for the flash will change when SPM is not built as a sub-image.

Requirements
************

The following development board:

* |nRF9160DK|

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/spm`

.. include:: /includes/build_and_run.txt

The sample is built as a secure firmware image for the nrf9160_pca10090 board.
See `Automatic building of SPM`_ if you want to program it independently from the non-secure application firmware.


Testing
=======

Program both the sample and your application firmware to the board.
After power-up, the sample starts your application firmware.

Observe that the application firmware operates as expected.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_spm`
