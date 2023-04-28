.. _fem_al_lib:

FEM abstraction layer
#####################

.. contents::
   :local:
   :depth: 2

This library is a wrapper for the MPSL FEM.

Overview
********

The library simplifies low level usage of FEM hardware.

Configuration
*************

To enable this library, use the :kconfig:option:`CONFIG_FEM_AL_LIB`.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`radio_test`
* :ref:`direct_test_mode`

Dependencies
************

* :ref:`mpsl_lib`

API documentation
*****************

| Header file: :file:`include/fem_al/fem_al.h`
| Source files: :file:`lib/fem_al/`

.. doxygengroup:: fem_al
   :project: nrf
   :members:
