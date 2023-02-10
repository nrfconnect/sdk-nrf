.. _security:

Security
########

.. contents::
   :local:
   :depth: 2

The following sections give a brief introduction to core security features available in Nordic Semiconductor products.
The features are made available either as built-ins in modules, drivers, and subsystems, or are shown in samples or applications in |NCS|.

Secure boot
***********

The |NCS| supports secure boot of application images enforced by the bootloader.
The secure boot utilizes signature validation and security hardware features to establish a root-of-trust during the boot process, and to ensure the validity of the firmware that is booted.

There are two available bootloaders in |NCS|:

* :ref:`nRF Secure Immutable Bootloader (NSIB) <bootloader>`, enabled by the configuration option :kconfig:option:`CONFIG_SECURE_BOOT`.
* A customized version of `sdk-mcuboot`_, enabled by the configuration option :kconfig:option:`CONFING_BOOTLOADER_MCUBOOT`.

When enabling the NSIB, the MCUboot can serve as an upgradable second-stage bootloader.
For more information about the bootloaders, see :ref:`app_bootloaders`.

Trusted Firmware-M
******************

The Trusted Firmware-M project (TF-M) is a reference design of the Arm `Platform Security Architecture (PSA)`_.
Through TF-M, |NCS| utilizes the security features of the Arm TrustZone technology to configure secure peripherals and memory, and to provide PSA functional APIs as secure services.

TF-M enables hardware supported separation of a Secure Processing Environment (SPE) and a Non-Secure Processing Environment (NSPE) that constitutes the Zephyr RTOS, protocol stacks, and the application.
Enable TF-M in a project by enabling the :kconfig:option:`CONFIG_BUILD_WITH_TFM` option.

For more information about the TF-M, see the `TF-M documentation`_ and :ref:`ug_tfm`.
For more information about SPE and NSPE in the |NCS|, see :ref:`app_boards_spe_nspe`.

Hardware unique key
*******************

Nordic Semiconductor devices featuring the CryptoCell cryptographic accelerator allow the usage of a hardware unique key (HUK) for key derivation.
A HUK is a unique symmetric cryptographic key which is loaded in special hardware registers allowing the application to use the key by reference, without any access to the key material.
To enable the HUK in an application, enable the :kconfig:option:`CONFIG_HW_UNIQUE_KEY` option.

For more information, see the hardware unique key :ref:`library <lib_hw_unique_key>` and :ref:`sample <hw_unique_key_usage>`.

Device firmware upgrade (DFU)
*****************************

The |NCS| supports firmware upgrade using over-the-air (OTA) and serial firmware upgrades, depending on the capabilities of the device.
For more information about the firmware upgrades, see the available :ref:`DFU libraries <lib_dfu>`.

The |NCS| can be configured to enforce secure DFU mechanisms, including validating the digital signature of an image and checking for the version to prevent downgrade attacks.
The secure DFU mechanisms are handled by the MCUboot bootloader.
For more information, see the :doc:`MCUboot documentation <mcuboot:design>`.

Cryptographic operations in |NCS|
*********************************

Cryptographic operations in |NCS| are handled by the :ref:`nrfxlib:nrf_security`, which is configurable through Kconfig options.
The module can be enabled through the :kconfig:option:`CONFIG_NRF_SECURITY` Kconfig option, and it allows the usage of `Mbed TLS`_ and `PSA Cryptography API 1.1`_ for cryptographic operations and random number generation in the application.

The :ref:`nrfxlib:nrf_security` acts as an orchestrator for the different cryptographic libraries available in the system.
These libraries include the binary versions of accelerated cryptographic libraries listed in :ref:`nrfxlib:crypto`, and the open source Mbed TLS implementation in |NCS| located in `sdk-mbedtls`_.

The Kconfig option :kconfig:option:`CONFIG_NRF_SECURITY` prioritizes the usage of the accelerated libraries by default when this is supported by the platform.
For more information about the configuration and usage of the :ref:`nrfxlib:nrf_security`, see the :ref:`nrfxlib:nrf_security_config` page.

See also :ref:`crypto_samples` for examples of how to configure and perform different cryptographic operations in the |NCS|.
