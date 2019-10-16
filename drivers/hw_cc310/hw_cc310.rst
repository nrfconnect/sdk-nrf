.. _lib_hw_cc310:

Hardware CC310 Driver
#####################

The Hardware CC310 Driver ensures correct initialization of the :ref:`nrf_cc310_platform_readme`.

The Hardware CC310 Driver ensures the following functionality is correctly initialized:

* CC310 abort functions
* mutex initialization
* platform initialization with/without RNG

The following :option:`CONFIG_HW_CC310` Kconfig variable controls initialization of the Hardware
CC310 Driver.

API documentation
*****************

| Source file: :file:`drivers/hw_cc310/hw_cc310.c`

See the :ref:`crypto_api_nrf_cc310_platform` APIs for functions available after
`Hardware CC310 Driver`_ initialization.
