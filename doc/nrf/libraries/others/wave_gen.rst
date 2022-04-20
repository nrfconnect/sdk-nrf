.. _wave_gen:

Wave generator
##############

.. contents::
   :local:
   :depth: 2

The wave generator is a simple library that generates the value of a wave signal.
The wave signal parameters are defined as :c:struct:`wave_gen_param`.
The :c:func:`wave_gen_generate_value` generates the value of the wave signal at a given time.

Configuration
*************

Set :kconfig:option:`CONFIG_WAVE_GEN_LIB` to enable the wave generator library.

API documentation
*****************

| Header file: :file:`include/wave_gen.h`
| Source files: :file:`lib/wave_gen/`

.. doxygengroup:: wave_gen
   :project: nrf
   :members:
