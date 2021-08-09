.. _ug_tfm:

Running applications with Trusted Firmware-M
############################################

.. contents::
   :local:
   :depth: 2

On nRF5340 and nRF9160, you can use Trusted Firmware-M (TF-M) as an alternative to :ref:`secure_partition_manager` for running an application from the non-secure area of the memory.

Overview
********

TF-M is the reference implementation of `Platform Security Architecture (PSA)`_.

It provides a highly configurable set of software components to create a Trusted Execution Environment.
This is achieved by a set of secure run time services such as Secure Storage, Cryptography, Audit Logs, and Attestation.
Additionally, secure boot via MCUboot in TF-M ensures integrity of run time software and supports firmware upgrade.

Support for TF-M in |NCS| is currently experimental.
TF-M is a framework which will be extended for new functions and use cases beyond the scope of SPM.

For official documentation, see `TF-M documentation`_.

The TF-M implementation in |NCS| is currently demonstrated in the following samples:

- The :ref:`tfm_hello_world` sample
- All :ref:`cryptography samples <crypto_samples>` in this SDK
- A series of :ref:`TF-M integration samples <zephyr:tfm_integration-samples>` available in Zephyr

Building
********

TF-M is one of the images that are built as part of a multi-image application.
If TF-M is used in the application, SPM will not be included in it.
For more information about multi-image builds, see :ref:`ug_multi_image`.

To add TF-M to your build, enable the :kconfig:`CONFIG_BUILD_WITH_TFM` configuration option by adding it to your :file:`prj.conf` file.
To build a :ref:`minimal version <tfm_minimal_build>` of TF-M, you must also enable the :kconfig:`CONFIG_TFM_MINIMAL` configuration.

.. note::
   If you use menuconfig to enable :kconfig:`CONFIG_BUILD_WITH_TFM`, you must also enable its dependencies.

You must build TF-M using a non-secure build target.
The following targets are currently supported:

* ``nrf5340dk_nrf5340_cpuapp_ns``
* ``nrf9160dk_nrf9160_ns``

When building for ``nrf9160dk_nrf9160_ns``, UART1 must be disabled in the non-secure application, because it is used by the TF-M secure application.
Otherwise, the non-secure application will fail to run.
The recommended way to do this is to copy the .overlay file from the :ref:`tfm_hello_world` sample.

Enabling secure services
========================

When using the :ref:`nrfxlib:nrf_security`, if :kconfig:`CONFIG_BUILD_WITH_TFM` is enabled together with :kconfig:`CONFIG_NORDIC_SECURITY_BACKEND`, the TF-M secure image will enable the use of the hardware acceleration of Arm CryptoCell.
In such case, the Kconfig configurations in the Nordic Security Backend control the features enabled through TF-M.

You can configure what crypto modules to include in TF-M by using the ``TFM_CRYPTO_`` Kconfig options found in file :file:`zephyr/modules/trusted-firmware-m/Kconfig.tfm.crypto_modules`.

TF-M utilizes :ref:`hardware unique keys <lib_hw_unique_key>` when the PSA Crypto key derivation APIs are used, and ``psa_key_derivation_setup`` is called with the algorithm ``TFM_CRYPTO_ALG_HUK_DERIVATION``.

.. _tfm_minimal_build:

Minimal build
=============

The default configuration of TF-M has all supported features enabled, which results in a significant memory footprint.
A minimal version of the TF-M secure application is provided in |NCS| to show how to configure a reduced version of TF-M.

The secure services supported by this minimal version allow for generating random numbers, hashing with SHA-256, and using ``tfm_platform_mem_read``.
This corresponds to the feature set provided by the :ref:`secure_partition_manager`.


The minimal version of TF-M is enabled by setting the :kconfig:`CONFIG_TFM_MINIMAL` option.

When :kconfig:`CONFIG_TFM_MINIMAL` is set, the configurability of TF-M is severely limited.
Hence, it is not possible to modify the TF-M minimal configuration to create your own variant of the minimal configuration.
Instead, the default configuration must be used as a starting point.

Programming
***********

The procedure for programming an application with TF-M is the same as for other multi-image applications in |NCS|.

After building the application, a :file:`merged.hex` file is created that contains MCUboot, TF-M, and the application.
The :file:`merged.hex` file can be then :ref:`programmed using SES <gs_programming_ses>`.
When using the command line, the file is programmed automatically when you call ``ninja flash`` or ``west flash``.

Logging
*******

TF-M employs two UART interfaces for logging: one for the secure part (MCUboot and TF-M), and one for the non-secure application.
The logs arrive on different COM ports on the host PC.

On the nRF5340 DK, you must connect specific wires on the kit to receive secure logs on the host PC.
Wire the pins P0.25 and P0.26 to RxD and TxD respectively.

Limitations
***********

Application code that uses SPM :ref:`lib_secure_services` cannot use TF-M because the interface to TF-M is different and, at this time, not all SPM functions are available in TF-M.
