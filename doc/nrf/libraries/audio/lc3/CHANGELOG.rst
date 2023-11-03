.. _lc3_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All notable changes to this project are documented on this page.

nRF Connect SDK v2.2.0
**********************

Here are all the notable changes included in the |NCS| v2.2.0 release:

* Added new documentation pages.
* Replaced API documentation link with the :ref:`lc3_api` page that includes content generated from sources.

nRF Connect SDK v2.1.0
**********************

Initial release in the sdk-nrxlib repository.
This release includes the following changes to the codec.

LC3 v1.0.4
==========

Build information:

* Platform: ARM Cortex-M33, Cortex-M44
* Compiler: ARM v6, -Ofast
* Hard floating point, fixed point

Changes:

* Replaced floating point stdlib dependencies in data path with T2 functions.
* Optimized the LTPF decoder, fixed point.

LC3 v1.0.3
==========

Changes:

* MDCT optimizations.
* LTPF optimizations.
* Arithmetic encoding optimizations.

LC3 v1.0.2
==========

Changes:

* Platform-specific optimizations.

LC3 v1.0.1
==========

Changes:

* Fixed point optimizations.

LC3 v1.0.0
==========

Changes:

* ARC LTPF SIMD optimizations.

LC3 v0.9.14
===========

Changes:

* ARM optimizations.

LC3 v0.9.13
===========

Changes:

* Added compile option to separate encode/decode memory.

LC3 v0.9.12
===========

Changes:

* ARM optimizations.

LC3 v0.9.11
===========

Changes:

* ARM optimizations.

LC3 v0.9.10
===========

Changes:

* Incorporated all technical errata up to 15138.

LC3 v0.9.9
==========

Changes:

* Non-functional changes.

LC3 v0.9.8
==========

Changes:

* Fixed point memory optimizations.
* Fixed point 7.5 ms frame size conformance improvements, meets stricter 0.06 ODG threshold in conformance tests.
* Fixed scaling for > 16 bit PCM output in fixed point.

LC3 v0.9.7
==========

Changes:

* Fixed Glockenspiel 24 kHz / 48 kbps / 7.5 ms exceeding 0.06 ODG in floating point encode/decode test.
* Performance optimizations.

LC3 v0.9.6
==========

Changes:

* Added 7.5 ms support for floating point.

LC3 v0.9.5
==========

Changes:

* Fixed point memory optimizations.

LC3 v0.9.4
==========

Changes:

* Added 24 and 32 bit PCM input and output to T2_LC3.
* Optimizations in pitch lag detection.
* Added API for amount of packet loss correction (PLC) applied.
* Added API for providing static buffers.
* Added error code offset to avoid value conflict with other modules.
* Added API for the number of bytes read from encoder input.

LC3 v0.9.3
==========

Changes:

* Fixed an issue in T2_LC3 where decoding a single frame resulted in 1/4 frame of additional PCM output.

LC3 v0.9.2
==========

Changes:

* Added x64 Linux build T2_LC3.
* Added support for multi-channel WAV files to T2_LC3 application.
* Removed a DLL dependency from T2_LC3 Windows build.
