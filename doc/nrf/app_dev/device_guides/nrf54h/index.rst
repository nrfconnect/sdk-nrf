.. _ug_nrf54h:

Developing with nRF54H Series
#############################

.. note::

   All software for the nRF54H20 SoC is experimental, and hardware availability is restricted to the participants in the customer sampling program.

.. |nrf_series| replace:: devices of the nRF54H Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support and contain board definitions for developing on the following nRF54H Series device:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Build target
   * - :zephyr:board:`nrf54h20dk`
     - PCA10175
     - | ``nrf54h20dk/nrf54h20/cpuapp``
       | ``nrf54h20dk/nrf54h20/cpurad``
       | ``nrf54h20dk/nrf54h20/cpuppr``

.. note::
   For details on the compatibility between nRF54H20 IronSide SE binaries and |NCS| versions, see :ref:`abi_compatibility`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_gs
   ug_nrf54h20_architecture
   ug_nrf54h20_configuration
   ug_nrf54h20_keys
   ug_nrf54h20_logging
   ug_nrf54h20_debugging
   ug_nrf54h20_custom_pcb
   ug_nrf54h20_ironside
   ug_nrf54h20_flpr
   ug_nrf54h20_ppr
   ../nrf54l/zms.rst
   ug_nrf54h20_mcuboot_dfu
   ug_nrf54h_ecies_x25519
   ug_nrf54h20_pm_optimization
   ug_nrf54h20_architecture_pinmap
