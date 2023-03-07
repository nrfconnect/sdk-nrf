.. _lib_tone:

Tone generator
##############

.. contents::
   :local:
   :depth: 2

The tone generator library creates an array of pulse-code modulation (PCM) data of a one-period sine tone, with a given tone frequency and sampling frequency.
For more information, see `API documentation`_.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_TONE` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

API documentation
*****************

| Header file: :file:`include/tone.h`
| Source file: :file:`lib/tone/tone.c`

.. doxygengroup:: tone_gen
   :project: nrf
   :members:
