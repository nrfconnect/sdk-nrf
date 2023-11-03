.. _crypto_changelog_cc310:

Changelog - nrf_cc310_bl
########################

.. contents::
   :local:
   :depth: 2

All notable changes to this project are documented in this file.

nrf_cc310_bl - 0.9.12
*********************

Initial release.

Added
=====

Added the following crypto libraries for cc310 bootloader nRF52840 and nRF91 Series.

.. note::
   * The cc310 libraries require the CryptoCell 310 HW accelerator, available on nRF52840 and nRF91 Series devices.
   * short-wchar: These libraries are compiled with a wchar_t size of 16 bits.
   * no-interrupts: These libraries use blocking waits for long-running cryptography calculations.


* nrf_cc310_bl, nRF91 Series variants:

  * ``cortex-m33/hard-float/libnrf_cc310_bl_0.9.12.a``
  * ``cortex-m33/hard-float/no-interrupts/libnrf_cc310_bl_0.9.12.a``
  * ``cortex-m33/soft-float/libnrf_cc310_bl_0.9.12.a``
  * ``cortex-m33/soft-float/no-interrupts/libnrf_cc310_bl_0.9.12.a``

  * short-wchar:

    * ``cortex-m33/hard-float/short-wchar/libnrf_cc310_bl_0.9.12.a``
    * ``cortex-m33/hard-float/short-wchar/no-interrupts/libnrf_cc310_bl_0.9.12.a``
    * ``cortex-m33/soft-float/short-wchar/libnrf_cc310_bl_0.9.12.a``
    * ``cortex-m33/soft-float/short-wchar/no-interrupts/libnrf_cc310_bl_0.9.12.a``

  * Keil:

    * ``cortex-m33/hard-float/short-wchar/nrf_cc310_bl_0.9.12.lib``
    * ``cortex-m33/hard-float/short-wchar/no-interrupts/nrf_cc310_bl_0.9.12.lib``
    * ``cortex-m33/soft-float/short-wchar/nrf_cc310_bl_0.9.12.lib``
    * ``cortex-m33/soft-float/short-wchar/no-interrupts/nrf_cc310_bl_0.9.12.lib``

* nrf_cc310_bl, nr52840 variants:

  * ``cortex-m4/hard-float/libnrf_cc310_bl_0.9.12.a``
  * ``cortex-m4/hard-float/no-interrupts/libnrf_cc310_bl_0.9.12.a``
  * ``cortex-m4/soft-float/libnrf_cc310_bl_0.9.12.a``
  * ``cortex-m4/soft-float/no-interrupts/libnrf_cc310_bl_0.9.12.a``

  * short-wchar:

    * ``cortex-m4/hard-float/short-wchar/libnrf_cc310_bl_0.9.12.a``
    * ``cortex-m4/hard-float/short-wchar/no-interrupts/libnrf_cc310_bl_0.9.12.a``
    * ``cortex-m4/soft-float/short-wchar/libnrf_cc310_bl_0.9.12.a``
    * ``cortex-m4/soft-float/short-wchar/no-interrupts/libnrf_cc310_bl_0.9.12.a``

  * Keil:

    * ``cortex-m4/hard-float/short-wchar/nrf_cc310_bl_0.9.12.lib``
    * ``cortex-m4/hard-float/short-wchar/no-interrupts/nrf_cc310_bl_0.9.12.lib``
    * ``cortex-m4/soft-float/short-wchar/nrf_cc310_bl_0.9.12.lib``
    * ``cortex-m4/soft-float/short-wchar/no-interrupts/nrf_cc310_bl_0.9.12.lib``
