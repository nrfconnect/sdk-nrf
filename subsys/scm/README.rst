.. _secure_context_manager:

Secure Context Manager
#################################

The Secure Context Manager (SCM) provides a reference implementation of a
secure partition configuration through the use of SPU.
The SCM is required to set up the nRF9160 DK so that it can run user
applications in the non-secure domain.

Overview
********

The SCM configures secure attributions of the SPU peripheral and jumps into
the non-secure application.

It configures security attributions for flash, SRAM, and peripherals.
After the configuration setup is complete, SCM jumps to the application firmware
that is located on the device.

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

If your application requires a different security attribution configuration, you must update the SCM code to reflect this.

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

This subsys can be found under :file:`subsys/spm` in the |NCS| folder structure.

The subsys is built as a secure firmware image for the nrf9160_pca10090 board.
It can be programmed independently from the non-secure application firmware.

See :ref:`gs_programming` for information about how to build and program the
subsys.

Testing
=======

Program both SCM and your application firmware to the board. After power-up, the SCM starts your application firmware.

Observe that the application firmware operates as expected.
