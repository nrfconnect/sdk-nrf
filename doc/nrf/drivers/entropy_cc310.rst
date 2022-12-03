.. _lib_entropy_cc310:

CC310 entropy driver
####################

.. contents::
   :local:
   :depth: 2

You can use the CC310 entropy driver (*entropy_cc310*) to generate random data using the Arm CryptoCell CC310 hardware available on the following Nordic SoCs/SiPs:

* nRF52840
* nRF5340
* nRF9160

When used with nRF52840, the CC310 entropy driver gathers entropy by using the CC310 hardware through the :ref:`nrf_cc3xx_platform_readme`.
When used with nRF5340 or nRF9160, the driver behavior depends on whether it is used in an application with or without Cortex-M Security Extensions (CMSE):

* In an application without CMSE the CC310 entropy driver gathers entropy by using the CC310 hardware through the :ref:`nrf_cc3xx_platform_readme`.
* In an application with CMSE, the driver gathers entropy through the PSA Crypto APIs exposed by :ref:`Trusted Firmware-M (TF-M) <ug_tfm>`.

For more details on CMSE, see :ref:`app_boards_spe_nspe`.

API documentation
*****************

| Header file: :file:`zephyr/include/drivers/entropy.h` (in the |NCS| project)
| Source file: :file:`drivers/entropy/entropy_cc310.c`

The entropy_cc3xx driver implements the Zephyr :ref:`zephyr:entropy_api` API.
