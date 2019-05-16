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
Modify the source code of the SPM subsystem to configure the security attributions of SRAM.
If Partition Manager is used, the security attributions of the flash regions are deduced from the generated file :file:`pm.config`.
Otherwise, the security attributions of the flash regions are deduced from Device Tree information.

For SRAM and peripherals, the following security attribution configuration is applied:

SRAM (256 kB)
   * Lower 64 kB: Secure
   * Upper 192 kB: Non-Secure

Peripherals configured as Non-Secure
   * CLOCK
   * EGU1, EGU2
   * FPU
   * GPIO (and GPIO pins)
   * GPIOTE1
   * IPC
   * NVMC, VMC
   * RTC1
   * SAADC
   * SPIM3
   * TIMER0-2
   * TWIM2
   * UARTE0, UARTE1

API documentation
*****************

| Header file: :file:`include/spm.h`
| Source files: :file:`subsys/spm/`

.. doxygengroup:: secure_partition_manager
   :project: nrf
   :members:
