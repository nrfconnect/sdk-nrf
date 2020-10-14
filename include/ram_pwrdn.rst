.. _lib_ram_pwrdn:

RAM Power Down
##############

.. contents::
   :local:
   :depth: 2

The RAM Power Down library is a basic module for disabling unused sections of RAM, which allows to save power in low power applications.

To disable unused RAM sections, call :c:func:`power_down_unused_ram`.
This function will automatically disable all memory regions that are not used by the application.

.. note::
    :c:func:`power_down_unused_ram` powers down memory regions that are outside of the ``_image_ram_end`` boundary.
    Accessing RAM that is powered down results in a bus fault exception.
    Working with an indirectly accessed memory that is not associated with any variable or a memory segment does not cause the ``_image_ram_end`` symbol to increase its value.
    For this reason, do not access powered down RAM sections when using this power optimization method.

API documentation
*****************

| Header file: :file:`include/ram_pwrdn.h`
| Source files: :file:`lib/ram_pwrdn/`

.. doxygengroup:: ram_pwrdn
   :project: nrf
   :members:
