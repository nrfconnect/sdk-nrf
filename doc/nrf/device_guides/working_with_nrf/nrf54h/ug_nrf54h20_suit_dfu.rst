.. _ug_nrf54h20_suit_dfu:

Device Firmware Update using SUIT
#################################

.. contents::
   :local:
   :depth: 2

The Software Updates for Internet of Things (SUIT) is the Device Firmware Upgrade (DFU) and secure boot method for nRF54H Series of System-on-Chip.
The SUIT procedure features a script-based system that resides in a :ref:`SUIT manifest <ug_nrf54h20_suit_manifest_overview>`.
The manifest contains instructions for the DFU and booting procedure, and can be customized.
It includes the use of :ref:`sequences <ug_suit_dfu_suit_concepts_sequences>` (built from conditions and directives) as well as the concept of :ref:`components <ug_suit_dfu_component_def>`.

To use the SUIT DFU in a product you need to customize it.
You can learn about this in the :ref:`ug_nrf54h20_suit_customize_dfu` page, which involves using the :ref:`nrf54h_suit_sample` sample.

For a list of available SUIT samples, see the :ref:`suit_samples` page.


.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_suit_intro.rst
   ug_nrf54h20_suit_manifest_overview.rst
   ug_nrf54h20_suit_customize_qsg.rst
   ug_nrf54h20_suit_customize_dfu.rst
   ug_nrf54h20_suit_fetch
   ug_nrf54h20_suit_external_memory
   ug_nrf54h20_suit_components.rst
   ug_nrf54h20_suit_hierarchical_manifests.rst
   ug_nrf54h20_suit_compare_other_dfu.rst
