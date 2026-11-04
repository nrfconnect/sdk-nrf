.. _lib_ram_pwrdn:

RAM power-down
##############

.. contents::
   :local:
   :depth: 2

The RAM power-down library powers down unused sections of RAM to reduce power consumption in low-power applications.

Overview
********

The library powers down RAM in complete hardware sections, which are the smallest units of RAM that the hardware can power down.
Powering a section down erases its contents.
Powering it up again does not restore them.
Accessing a powered-down section causes a bus fault.

Implementation
==============

The :c:func:`power_down_unused_ram` function powers down a RAM section only if the section is inside the ``zephyr,sram`` chosen devicetree node and no byte of it belongs to one of the following memory ranges:

* The linked application image, which is all RAM below the ``_image_ram_end`` symbol.
  This includes stacks, the system heap, and heaps defined with :c:macro:`K_HEAP_DEFINE`.
* An enabled child node of the ``/reserved-memory`` devicetree node.
  For example, the IPC shared memory for communication with the Wi-Fi core or the FLPR core.
* An ``mmio-sram`` devicetree node with ``status = "reserved"``.
  For example, the code and data memory of the FLPR core.

The build system generates this list from the devicetree.

In builds with Trusted Firmware-M, the library powers down RAM through the TF-M RAM control service.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_RAM_POWER_DOWN_LIBRARY` Kconfig option.

To power down unused RAM during system initialization without any application code, enable the :kconfig:option:`CONFIG_RAM_POWER_DOWN_AUTO` Kconfig option.
This option is available only with the minimal C library or picolibc, and only if the :kconfig:option:`CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE` Kconfig option is set to ``0`` or a positive value.
In that case, the C library heap has a fixed size and is part of the linked application image.

Usage
*****

To power down unused RAM sections, call the :c:func:`power_down_unused_ram` function.
To power them up again, call the :c:func:`power_up_unused_ram` function.

If your application uses RAM inside the ``zephyr,sram`` chosen devicetree node but outside the memory ranges listed in `Implementation`_, the library powers down that RAM.

Limitations
***********

Some C libraries place the ``malloc()`` heap in the RAM that follows the application image.
For example, the common C library does this when the :kconfig:option:`CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE` Kconfig option is set to ``-1``.
This heap is not in any of the memory ranges that the library keeps powered, so do not use the library with such a heap.

Dependencies
************

In builds with Trusted Firmware-M, the RAM power-down library uses the TF-M RAM control service.

API documentation
*****************

| Header file: :file:`include/ram_pwrdn.h`
| Source files: :file:`lib/ram_pwrdn/`

.. doxygengroup:: ram_pwrdn
