.. _lib_contin_array:

Continuous array
################

.. contents::
   :local:
   :depth: 2

The continuous array library introduces an array that you can loop over, for example if you want to create a period of a Pulse-code modulated (PCM) sine wave.
You can use it to test playback with applications that support audio development kits, for example the :ref:`nrf53_audio_app`.

The library introduces the :c:func:`contin_array_create` function, which takes an array that the user wants to loop over.
For more information, see `API documentation`_.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_CONTIN_ARRAY` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

API documentation
*****************

| Header file: :file:`include/contin_array.h`
| Source file: :file:`lib/contin_array/contin_array.c`

.. doxygengroup:: contin_array
   :project: nrf
   :members:
