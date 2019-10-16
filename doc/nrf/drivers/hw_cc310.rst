.. _lib_hw_cc310:

CC310 hardware driver
#####################

The CC310 hardware driver (hw_cc310) ensures correct initialization of the :ref:`nrf_cc310_platform_readme`.

It initializes the following elements of the library:

* CC310 abort functions,
* hardware mutex and mutex API,
* platform with or without RNG.

You can initialize the hw_cc310 driver using the :option:`CONFIG_HW_CC310` Kconfig option.

API documentation
*****************

| Source file: :file:`drivers/hw_cc310/hw_cc310.c`

After the hw_cc310 driver has been initialized, you can use the APIs from the :ref:`crypto_api_nrf_cc310_platform` and the :ref:`nrf_cc310_mbedcrypto_readme`.
