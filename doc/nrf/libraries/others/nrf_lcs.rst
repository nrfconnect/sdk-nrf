.. _lib_nrf_lcs:

nRF Life Cycle State
####################

.. contents::
   :local:
   :depth: 2

The nRF Life Cycle State (LCS) library provides an PSA-compatible API for Nordic devices.
This API allows you to both read and update the LCS.

The API is designed to be used by the bootloader, but it can also be used by other components as well, as long as the underlying storage implementation is available.

The LCS names and descriptions are compliant with the PSA specification, yet are extended by expectations and steps performed by the firmware, which are based on Nordic design decisions.

The constants representing the LCS values are defined in such a way, that a simple skip, uninitialized variable or a stack manipulation attack is likely to result in an invalid LCS value, which can be detected by the calling API.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_NRF_LCS` Kconfig option to y in the project configuration file :file:`prj.conf`.

You can configure one of the following Kconfig options to choose the memory backend for the LCS storage:

* :kconfig:option:`CONFIG_NRF_LCS_MEM_BL_STORAGE` - This option specifies the bootloader storage as the memory backend for the LCS.

API documentation
*****************

| Header file: :file:`include/nrf_lcs/nrf_lcs.h`
| Source files: :file:`subsys/nrf_lcs/`

.. doxygengroup:: nrf_lcs
