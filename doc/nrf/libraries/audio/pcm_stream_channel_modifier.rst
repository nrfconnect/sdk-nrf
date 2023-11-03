.. _lib_pcm_stream_channel_modifier:

PCM Stream Channel Modifier
###########################

.. contents::
   :local:
   :depth: 2

PCM Stream Channel Modifier library enables users to split pulse-code modulation (PCM) streams from stereo to mono or combine mono streams to form a stereo stream.
For more information, see `API documentation`_.

Configuration
*************

To enable the library, set the :kconfig:option:`CONFIG_PSCM` Kconfig option to ``y`` in the project configuration file :file:`prj.conf`.

API documentation
*****************

| Header file: :file:`include/pcm_stream_channel_modifier.h`
| Source file: :file:`lib/pcm_stream_channel_modifier/pcm_stream_channel_modifier.c`

.. doxygengroup:: pscm
   :project: nrf
   :members:
