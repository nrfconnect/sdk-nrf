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

The TF-M implementation in |NCS| is currently demonstrated in the :ref:`tfm_hello_world` sample.

Building
********

TF-M is one of the images that are built as part of a multi-image application.
If TF-M is used in the application, SPM will not be included in it.
For more information about multi-image builds, see :ref:`ug_multi_image`.

To add TF-M to your build, enable the :option:`CONFIG_BUILD_WITH_TFM` configuration option by adding it to your :file:`prj.conf` file.

.. note::
   If you use menuconfig to enable :option:`CONFIG_BUILD_WITH_TFM`, you must also enable its dependencies.

You must build TF-M using a non-secure build target.
The following targets are currently supported:

* ``nrf5340dk_nrf5340_cpuappns``
* ``nrf9160dk_nrf9160ns``

When building for ``nrf9160dk_nrf9160ns``, UART1 must be disabled in the non-secure application, because it is used by the TF-M secure application.
Otherwise, the non-secure application will fail to run.
The recommended way to do this is to copy the .overlay file from the :ref:`tfm_hello_world` sample.

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
