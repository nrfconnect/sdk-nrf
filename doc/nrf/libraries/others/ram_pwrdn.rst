.. _lib_ram_pwrdn:

RAM power-down
##############

.. contents::
   :local:
   :depth: 2

The RAM power-down library is a basic module for disabling unused sections of RAM, which allows to save power in low-power applications.

To disable unused RAM sections, call :c:func:`power_down_unused_ram`.
This function will automatically disable all memory regions that are not used by the application.
To enable back unused RAM sections, call :c:func:`power_up_unused_ram`.

.. note::
    :c:func:`power_down_unused_ram` powers down memory regions that are outside of the ``_image_ram_end`` boundary.
    Accessing RAM that is powered down results in a bus fault exception.
    Working with an indirectly accessed memory that is not associated with any variable or a memory segment does not cause the ``_image_ram_end`` symbol to increase its value.
    For this reason, do not access powered down RAM sections when using this power optimization method.

.. note::
   :c:func:`power_down_unused_ram` does not power down segments of RAM reserved by the :ref:`partition_manager`.
   If you need to reserve a segment of RAM, create your own RAM partition using the :ref:`partition_manager`.

API documentation
*****************

| Header file: :file:`include/ram_pwrdn.h`
| Source files: :file:`lib/ram_pwrdn/`

.. doxygengroup:: ram_pwrdn
   :project: nrf
   :members:
