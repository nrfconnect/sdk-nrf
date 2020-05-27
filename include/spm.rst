.. _lib_spm:

Secure Partition Manager (SPM)
##############################

The Secure Partition Manager (SPM) provides functionality for the Trusted Execution Environment of the nRF9160.

Overview
********

The Cortex-M33 CPU in the nRF9160 implements ARM TrustZone, which means it can run a "secure" and a "non-secure" app side by side.
The SPM, being the secure app, is responsible for configuring the permissions and resources of the non-secure app and then booting it.
Such configuration is required to run non-secure apps.
The SPM also provides the non-secure app with access to features (:ref:`lib_spm_secure_services`) that are normally only available to secure apps.

The SPM library is used in the :ref:`secure_partition_manager` sample.

.. _lib_spm_configuration:

Configuration
*************

The Secure Partition Manager (SPM) uses the SPU peripheral to configure security attributions for the flash, SRAM, and peripherals.
Note that the SPU peripheral is the nRF version of an IDAU (Implementation-Defined Security Attribution Unit).

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
   * DPPI
   * EGU1, EGU2
   * FPU
   * GPIO (and GPIO pins)
   * GPIOTE1
   * IPC
   * NVMC, VMC
   * PWM0-3
   * REGULATORS
   * RTC0, RTC1
   * SAADC
   * SPIM3
   * TIMER0-2
   * TWIM2
   * UARTE0, UARTE1
   * WDT

.. _lib_spm_secure_services:

Secure Services
***************

The SPM by default provides certain Secure Services to the Non-Secure Firmware. See :ref:`lib_secure_services` for more information.

API documentation
*****************

| Header file: :file:`include/spm.h`
| Source files: :file:`subsys/spm/`

.. doxygengroup:: secure_partition_manager
   :project: nrf
   :members:
