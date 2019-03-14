.. _lib_spm:

Secure Partition Manager (SPM)
##############################

The Secure Partition Manager (SPM) uses the SPU peripheral to configure security attributions for the flash, SRAM, and peripherals.
Note that the SPU peripheral is the nRF version of an IDAU (Implementation-Defined Security Attribution Unit).

It is used in the :ref:`secure_partition_manager` sample.

.. _lib_spm_configuration:

Configuration
*************

Use Kconfig to configure the security attributions for the peripherals.
Modify the source code of the SPM subsystem to configure the security attributions of flash or SRAM.

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

API documentation
*****************

.. doxygengroup:: secure_partition_manager
   :project: nrf
   :members:
