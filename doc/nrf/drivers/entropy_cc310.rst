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
When used with nRF5340 or nRF9160, the driver behavior depends on whether it is used in a secure or non-secure application:

* In a secure application, or when the :ref:`lib_spm` is not used, the CC310 entropy driver gathers entropy by using the CC310 hardware through the :ref:`nrf_cc3xx_platform_readme`.
* In a non-secure application, the driver gathers entropy through the :ref:`lib_secure_services` library.

For more details on secure and non-secure applications, see :ref:`ug_nrf5340` and :ref:`ug_nrf9160`.

API documentation
*****************

| Header file: :file:`zephyr/include/drivers/entropy.h` (in the |NCS| project)
| Source file: :file:`drivers/entropy_cc310/entropy_cc310.c`

The entropy_cc3xx driver implements the Zephyr :ref:`zephyr:entropy_api` API.
