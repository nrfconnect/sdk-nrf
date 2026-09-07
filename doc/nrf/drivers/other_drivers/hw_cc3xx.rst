.. _lib_hw_cc310:
.. _lib_hw_cc3xx:

CC3XX hardware driver
#####################

.. contents::
   :local:
   :depth: 2

The CC3XX hardware driver (hw_cc3xx) ensures correct initialization of the :ref:`nrf_cc3xx_platform_readme`.

It initializes the following elements of the library:

* CC3XX abort functions.
* Hardware mutex and mutex API.
* Platform with or without RNG.

You can initialize the hw_cc3xx driver using the :kconfig:option:`CONFIG_HW_CC3XX` Kconfig option.

API documentation
*****************

| Source file: :file:`drivers/hw_cc3xx/hw_cc3xx.c`

After the hw_cc3xx driver has been initialized, you can use the APIs from the :ref:`crypto_api_nrf_cc3xx_platform` and the :ref:`nrf_cc3xx_mbedcrypto_readme`.
