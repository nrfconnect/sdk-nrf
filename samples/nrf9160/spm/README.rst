.. _secure_partition_manager:

nRF9160: Secure Partition Manager
#################################

The Secure Partition Manager sample provides a reference use of the Secure Partition Manager peripheral.
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

Requirements for the application firmware
=========================================

* The application firmware must be located in the slot_ns flash partition.
  For more details, see the `nrf9160_pca10090_partition_conf.dts`_ file in the nrf9160_pca10090 board definition.
  Note that if you build your application firmware with the |NCS|, this requirement is automatically fulfilled.

* The application firmware must be built as a non-secure firmware for the nrf9160_pca10090ns board.

Requirements
************

The following development board:

* nRF9160 DK board (PCA10090)

Building and running
********************

This sample can be found under :file:`samples/nrf9160/spm` in the |NCS| folder structure.

The sample is built as a secure firmware image for the nrf9160_pca10090 board.
It can be programmed independently from the non-secure application firmware.

See :ref:`gs_programming` for information about how to build and program the application.

Testing
=======

Program both the sample and your application firmware to the board.
After power-up, the sample starts your application firmware.

Observe that the application firmware operates as expected.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_spm`
