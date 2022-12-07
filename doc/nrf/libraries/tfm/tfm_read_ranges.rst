.. _lib_tfm_read_ranges:

TF-M read ranges
################

.. contents::
   :local:
   :depth: 2

The TF-M read ranges library provides an API for reading of secure memory regions, like for example image metadata or factory information configuration registers (FICR).
It is one of the platform-specific Trusted Firmware-M (TF-M) services added by the ``platform`` partition.

The API is available for applications that implement Non-Secure Processing Environment (NSPE) alongside Secure Processing Environment (SPE).

For more information on implementing TF-M in your application, see :ref:`ug_tfm`.
For more information about NSPE and SPE, see :ref:`app_boards_spe_nspe`.

Requirements
************

This library requires that TF-M is installed on the device.

Configuration
*************

Set the :kconfig:option:`CONFIG_TFM_PARTITION_PLATFORM` Kconfig option to enable the use of this service and its library.

Dependencies
************

?

API documentation
*****************

| Header file: :file:`include/tfm/tfm_read_ranges.h`
| Source files: ?
