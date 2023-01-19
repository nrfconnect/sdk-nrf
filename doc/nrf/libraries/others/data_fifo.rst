.. _lib_data_fifo:

Data FIFO
#########

.. contents::
   :local:
   :depth: 2

This library combines the Zephyr memory slab and message queue mechanisms.
The purpose is to be able to allocate a memory slab, use it, and signal to a receiver when the write operation has completed.
The reader can then read and free the memory slab when done.
For more information, see `API documentation`_.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_DATA_FIFO` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

API documentation
*****************

| Header file: :file:`include/data_fifo.h`
| Source file: :file:`lib/data_fifo/data_fifo.c`

.. doxygengroup:: data_fifo
   :project: nrf
   :members:
