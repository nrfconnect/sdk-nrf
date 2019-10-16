.. _lib_entropy_cc310:

Entropy CC310 Driver
####################

The Entropy CC310 Driver can be used to to generate random data using the Arm
CryptoCell CC310 hardware.


When used in nRF52840 the Entropy CC310 Driver will gather entropy by using the
CC310 hardware through the :ref:`nrf_cc310_platform_readme`.


When used in the nRF9160 in a secure application, :ref:`ug_nrf9160`, or when the
Secure Partition Manager is not used, the Entropy CC310 Driver will gather
entropy by using the CC310 hardware through the :ref:`nrf_cc310_platform_readme`.


When used in the nRF9160 in a non-secure application, :ref:`ug_nrf9160`
the driver will gather entropy through the :ref:`lib_secure_services` library.

API documentation
*****************

| Header file: :file:`<NCS>/zephyr/include/drivers/entropy.h`
| Source file: :file:`drivers/entropy_cc310/entropy_cc310.c`

The Entropy CC310 Driver implements the Zephyr :ref:`zephyr:entropy_interface` API.
