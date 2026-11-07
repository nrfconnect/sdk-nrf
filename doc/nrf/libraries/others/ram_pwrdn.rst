.. _lib_ram_pwrdn:

RAM power-down
##############

.. contents::
   :local:
   :depth: 2

The RAM power-down library is a basic module for disabling unused sections of RAM, which allows to save power in low-power applications.

To disable unused RAM sections, call :c:func:`power_down_unused_ram`.
This function will automatically disable all memory regions that are not used by the application.
RAM sections that overlap a non-disabled ``/reserved-memory`` child node or an ``mmio-sram`` node with the ``reserved`` status are kept powered.
To enable back unused RAM sections, call :c:func:`power_up_unused_ram`.

.. note::
    :c:func:`power_down_unused_ram` powers down memory regions that are outside of the ``_image_ram_end`` boundary.
    Accessing RAM that is powered down results in a bus fault exception.
    Working with an indirectly accessed memory that is not associated with any variable or a memory segment does not cause the ``_image_ram_end`` symbol to increase its value.
    For this reason, do not access powered down RAM sections when using this power optimization method.

Automatic power-down
********************

Set the :kconfig:option:`CONFIG_RAM_POWER_DOWN_LIBRARY` and :kconfig:option:`CONFIG_RAM_POWER_DOWN_AUTO` Kconfig options to call :c:func:`power_down_unused_ram` during system initialization.
The option is only available with the minimal C library or picolibc, and not if the :kconfig:option:`CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE` Kconfig option is set to ``-1``, as the ``malloc()`` heap then uses the RAM after the application image.

API documentation
*****************

| Header file: :file:`include/ram_pwrdn.h`
| Source files: :file:`lib/ram_pwrdn/`

.. doxygengroup:: ram_pwrdn
