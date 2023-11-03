.. _lib_pcm_mix:

Pulse Code Modulation audio mixer
#################################

.. contents::
   :local:
   :depth: 2

The Pulse Code Modulation (PCM) audio mixer lets you mix a PCM stream into another PCM stream.
It can for example be useful for mixing a tone and an audio stream together.
This library is useful for developing applications that offer audio features, for example using the nRF5340 Audio DK.


The library offers the following types of mixing support:

* Stereo stream into another stereo stream
* Combinations of mono to mono
* Mono to stereo: channel left or right or left+right

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_PCM_MIX` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

API documentation
*****************

| Header file: :file:`include/pcm_mix.h`
| Source file: :file:`lib/pcm_mix/pcm_mix.c`

.. doxygengroup:: pcm_mix
   :project: nrf
   :members:
