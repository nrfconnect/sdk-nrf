.. _secure_boot:

nRF9160: Secure Boot
####################

The Secure Boot sample application provides a reference implementation of a first-stage boot firmware.
This firmware is required to set up the nRF9160 DK so that it can run user applications in the non-secure domain.

Overview
********

The sample configures secure attributions for the nRF9160 SiP and jumps into the non-secure application.

It utilizes the SPU peripheral to configure security attributions for the nRF9160 flash, SRAM, and peripherals.
After the configuration setup is complete, the sample loads the application firmware that is located on the device.

Security attribution configuration
==================================

The following security attribution configuration is applied:

Flash (1 Mb)
   * Lower 256 kB: Secure
   * Upper 768 kB: Non-Secure

SRAM (256 kB)
   * Lower 64 kB: Secure
   * Upper 192 kB: Non-Secure

Peripherals configured as Non-Secure
   * GPIO (and GPIO pins)
   * CLOCK
   * RTC1
   * IPC
   * NVMC, VMC
   * GPIOTE1
   * UARTE0, UARTE1
   * EGU1, EGU2
   * FPU

If your application requires a different security attribution configuration, you must update the Secure Boot sample code to reflect this.

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

This sample can be found under :file:`samples/nrf9160/secure_boot` in the |NCS| folder structure.

The sample is built as a secure firmware image for the nrf9160_pca10090 board.
It can be programmed independently from the non-secure application firmware.

See :ref:`gs_programming` for information about how to build and program the application.

Testing
=======

Program both the sample and your application firmware to the board. After power-up, the sample starts your application firmware.

Observe that the application firmware operates as expected.
