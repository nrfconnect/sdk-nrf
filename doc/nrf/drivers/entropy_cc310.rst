.. _lib_entropy_cc310:

CC310 entropy driver
####################

You can use the CC310 entropy driver (entropy_cc310) to generate random data using the Arm CryptoCell CC310 hardware.
This hardware is available on the nRF52840 and nRF9160 devices.

When used on nRF52840, the entropy_cc310 driver gathers entropy by using the CC310 hardware through the :ref:`nrf_cc310_platform_readme`.

When used on nRF9160, the driver behavior depends on whether it is used in a secure or non-secure application:

  * When used :ref:`in a secure application <ug_nrf9160>`, or when the :ref:`lib_spm` is not used, the entropy_cc310 driver gathers entropy by using the CC310 hardware through the :ref:`nrf_cc310_platform_readme`.

  * When used :ref:`in a non-secure application <ug_nrf9160>`, the driver gathers entropy through the :ref:`lib_secure_services` library.

API documentation
*****************

| Header file: :file:`<NCS>/zephyr/include/drivers/entropy.h`
| Source file: :file:`drivers/entropy_cc310/entropy_cc310.c`

The entropy_cc310 driver implements the Zephyr :ref:`zephyr:entropy_api` API.
