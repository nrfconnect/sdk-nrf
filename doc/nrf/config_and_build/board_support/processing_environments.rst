.. _app_boards_spe_nspe:

Processing environments
#######################

.. contents::
   :local:
   :depth: 2

The :ref:`boards supported by the SDK <app_boards_names>` distinguish entries according to the CPU to target (for multi-core SoCs) and whether Cortex-M Security Extensions (CMSE) are used or not (addition of the ``*/ns`` :ref:`variant <app_boards_names>` if they are used).

When CMSE is used, the firmware is split in accordance with the security by separation architecture principle to better protect sensitive assets and code.
With CMSE, the firmware is stored in one of two security environments (flash partitions), either Secure Processing Environment (SPE) or Non-Secure Processing Environment (NSPE).
This isolation of firmware is only possible if the underlying hardware supports `ARM TrustZone`_.

.. figure:: images/spe_nspe.svg
   :alt: Processing environments in the |NCS|

   Processing environments in the |NCS|

In Zephyr and the |NCS|, SPE and NSPE are used exclusively in the context of the application core of a multi-core SoC.
Building follows the security by separation principle and depends on the board target.

.. _app_boards_spe_nspe_cpuapp:

Building for ``cpuapp`` (CMSE disabled)
***************************************

When you build for a board target that uses the ``cpuapp`` :ref:`CPU cluster <app_boards_names>`, but does not use the ``*/ns`` :ref:`variant <app_boards_names>`, you build the firmware for the application core without CMSE.
Because CMSE is disabled, TF-M is not used and there is no separation of firmware.

.. _app_boards_spe_nspe_cpuapp_ns:

Building for ``*/ns`` (CMSE enabled)
************************************

When you build for a board target that uses the ``*/ns`` :ref:`variant <app_boards_names>`, you build firmware with CMSE.
Firmware is separated in the following way:

* SPE implements security-critical functionality and data (including bootloaders) and isolates them from the application software in NSPE.
  It also contains secure firmware running in the secure state.
* NSPE typically implements the user application and communication firmware, among other major components.

The application is built as a non-secure image and :ref:`Trusted Firmware-M (TF-M) <ug_tfm>` is built as the secure image.
The build system merges both images to form a combined image that will be used for programming or updating the device.

TF-M enables hardware-supported separation of firmware.
It also implements `Platform Security Architecture (PSA)`_ API, which provides security features for the system, including roots of trust for protecting secrets, platform state, and cryptographic keys.
The API coordinates the communication with the components in NSPE.

More information about SPE and NSPE
===================================

Read the following pages for a better understanding of security by separation in the |NCS|:

* :ref:`ug_tfm`
* :ref:`debugging_spe_nspe`
* `An Introduction to Trusted Firmware-M (TF-M)`_ on DevZone
* `TF-M documentation`_
