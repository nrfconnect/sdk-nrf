.. _gzll_changelog:

Changelog
#########

.. contents::
   :local:
   :depth: 2

All the notable changes to this project are documented on this page.

nRF Connect SDK v2.2.0
**********************

All the notable changes included in the |NCS| v2.2.0 release are documented in this section.

Added
=====

* Added the library variant for the nRF5340 network core.
* Added DPPI to glue layer.

Bug fixes
=========

* Fixed the :c:func:`nrf_gzll_init` function to avoid calling memcpy() with the same addresses.
* Applied workarounds for nRF5340 anomalies 117 and 158.

nRF Connect SDK v1.8.0
**********************

Initial release.

Added
=====

* Added the :file:`libgzll.a` library variant, in both soft-float and hard-float builds, for the following nRF52 Series SoCs:

  * nRF52810
  * nRF52811
  * nRF52820
  * nRF52832
  * nRF52833
  * nRF52840
