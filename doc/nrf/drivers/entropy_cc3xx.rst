.. _lib_entropy_cc310:
.. _lib_entropy_cc3xx:

CC3XX entropy driver
####################

.. contents::
   :local:
   :depth: 2

You can use the CC310 entropy driver (*entropy_cc3xx*) to generate random data using the Arm CryptoCell CC3XX hardware available on the following Nordic SoCs/SiPs:

* nRF52840
* nRF5340
* nRF91 Series

The CC3XX entropy driver gathers entropy by using the CC3XX hardware through the :ref:`nrf_cc3xx_platform_readme`.

API documentation
*****************

| Header file: :file:`zephyr/include/drivers/entropy.h` (in the |NCS| project)
| Source file: :file:`drivers/entropy/entropy_cc3xx.c`

The entropy_cc3xx driver implements the Zephyr :ref:`zephyr:entropy_api` API.
