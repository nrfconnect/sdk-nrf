:orphan:

.. _ug_nrf54h20_suit_dfu:

Device Firmware Update using SUIT
#################################

.. contents::
   :local:
   :depth: 2

The Software Updates for Internet of Things (SUIT) is a metadata format and procedure for performing device firmware updates (DFU).
SUIT was originally created by the `Internet Engineering Task Force <https://datatracker.ietf.org/wg/suit/about/>`__.
Nordic Semiconductor has created an implementation of the SUIT procedure that is the newly introduced DFU procedure for the nRF54H Series of System-on-Chip (SoC) and will be the only DFU procedure compatible with the nRF54H Series.

This SoC features a Secure Domain, which includes Secure Domain Firmware (SDFW) by default.
The Secure Domain will use the SUIT procedure to perform a DFU of the whole system.

To use Nordic Semiconductor's implementation of the SUIT procedure, see the ``nrf54h_suit_sample`` sample.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_suit_intro
   ug_nrf54h20_suit_manifest_overview.rst
   ug_nrf54h20_suit_hierarchical_manifests.rst
   ug_nrf54h20_suit_customize_dfu.rst
   ug_nrf54h20_suit_components.rst
