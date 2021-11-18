.. _lib_spm:

Secure Partition Manager (SPM)
##############################

.. contents::
   :local:
   :depth: 2

The Secure Partition Manager (SPM) provides functionality for the Trusted Execution Environment of the nRF9160 SiP and the nRF5340 SoC.

Overview
********

The Cortex-M33 CPU in the nRF9160 and nRF5340 devices implements `ARM TrustZone`_.
This allows the CPU to run a "secure" and a "non-secure" app side by side.

The SPM runs as a secure app.
It configures the permissions and resources of the non-secure app and then boots it.
Such configuration is required to run non-secure apps.

The SPM also provides the non-secure app with access to features that are normally only available to secure apps.
You can find the feature list on the :ref:`lib_spm_secure_services` page.

.. note::
   If your application uses :ref:`TF-M <ug_tfm>`, SPM is not included in the build.

The SPM library is used in the :ref:`secure_partition_manager` sample.

.. _lib_spm_configuration:

Configuration
*************

The Secure Partition Manager (SPM) uses the `System Protection Unit`_ (SPU) peripheral to set security attributions for the flash memory, the SRAM, and other peripherals.
Security attributions are boolean Kconfig options that configure security settings like, for example, defining a peripheral as secure or non-secure.
The SPU peripheral is the nRF version of an Implementation-Defined Security Attribution Unit (IDAU).

To configure SPM, do the following:

* Use Kconfig to configure the security attributions for the peripherals.
* Modify the source code of the SPM library to configure the security attributions of the SRAM.

Partition Manager must be used with SPM.
The security attributions of the flash memory regions are taken from the generated :file:`pm.config` file.

For the SRAM and the peripherals, the following security attribution configuration is applied:

SRAM (256 kB)
* Lower 64 kB: Secure
* Upper 192 kB: Non-Secure

The following peripherals are configured as Non-Secure:

* Common peripherals:

  * CLOCK
  * DPPI
  * EGU1, EGU2
  * FPU
  * GPIO (and GPIO pins)
  * GPIOTE1
  * IPC
  * NFCT
  * NVMC, VMC
  * PWM0-3
  * REGULATORS
  * RTC0, RTC1
  * SAADC
  * SPIM3
  * TIMER0-2
  * TWIM2
  * UARTE0, UARTE1
  * P0

* nRF9160 SiP specific peripherals:

  * WDT
  * PDM
  * I2S

* nRF5340 SoC specific peripherals:

  * Oscillators
  * Reset
  * SPIM4
  * WDT0-1
  * COMP
  * LPCOMP
  * PDM0
  * I2S0
  * QSPI
  * NFCT
  * MUTEX
  * QDEC0-1
  * USBD
  * USB Regulator
  * P1

.. _lib_spm_secure_services:

Secure Services
***************

The SPM provides by default certain Secure Services to the Non-Secure Firmware.
See :ref:`lib_secure_services` for more information.

API documentation
*****************

| Header file: :file:`include/spm.h`
| Source files: :file:`subsys/spm/`

.. doxygengroup:: secure_partition_manager
   :project: nrf
   :members:
