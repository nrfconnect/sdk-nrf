.. _emulated-nrf54l10:

nRF54L10 snippet
################

.. contents::
   :local:
   :depth: 2

Overview
********

This snippet (``emulated-nrf54l10``) emulates nRF54L10 on the nRF54L15 DK.

Supported SoCs and boards
*************************

.. warning:
   This snippet cannot be used with the FLPR core because all memory, including RAM and RRAM, is allocated to the application core.

Currently, the only SoC and board supported for use with the snippet is:

* :ref:`zephyr:nrf54l15dk_nrf54l15`
