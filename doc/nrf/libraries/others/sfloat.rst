.. _lib_sfloat:

Short float (SFLOAT)
####################

.. contents::
   :local:
   :depth: 2

The library provides an implementation of the SFLOAT type, which is used to encode health data like glucose concentration or blood pressure.

Overview
********

You can use the library to encode numbers that have the fractional part (for example ``10.2``).
This data type is similar to the standard FLOAT type ``float``, but it is more compact and has a lower resolution.

The IEEE 11073-20601-2008 specification defines the SFLOAT type.
This data type encodes each number in two bytes using the following format:

.. math::

   SFLOAT = 10^e * m

Where:

* ``e`` denotes exponent, encoded as a 4-bit integer in two's-complement form, value range ``-8 - 7``.
* ``m`` denotes mantissa, encoded as a 12-bit integer in two's-complement form, value range ``-2048 - 2047``.

The SFLOAT type typically encodes data used in health devices.
For instance, the Glucose Concentration field in the Bluetooth Continuous Glucose Monitoring service uses the SFLOAT type.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_SFLOAT` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

FLOAT conversion
****************

This library provides an API to convert the FLOAT type into SFLOAT type.
The API is only compatible with the FLOAT type from the IEEE 754 specification that uses the ``binary32`` parameter from the interchange format.
The conversion process may lead to some loss of information about the converted value as the SFLOAT type has a smaller encoding resolution than the FLOAT Type.
The library always preserves three significant figures of the original value.
Under additional conditions, it also preserves the fourth figure.
You can get four significant figures in the converted value when the four significant figures of the original value compose a number in the mantissa range: ``-2048 - 2047``.

Dependencies
************

* :file:`include/zephyr/sys/byteorder.h`

API documentation
*****************

| Header file: :file:`include/sfloat.h`
| Source file: :file:`lib/sfloat/sfloat.c`

.. doxygengroup:: sfloat
   :project: nrf
   :members:
