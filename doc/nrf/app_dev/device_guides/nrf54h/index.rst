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
   * - :ref:`zephyr:nrf54h20dk_nrf54h20`
     - PCA10175
     - | ``nrf54h20dk_nrf54h20_cpuapp``
       | ``nrf54h20dk_nrf54h20_cpurad``
       | ``nrf54h20dk_nrf54h20_cpuppr``

.. note::
   For details on the compatibility between nRF54H20 SoC binaries and |NCS| versions, see :ref:`abi_compatibility`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_gs
   ug_nrf54h20_architecture
   ug_nrf54h20_configuration
   ug_nrf54h20_suit_dfu
   ug_nrf54h20_keys
   ug_nrf54h20_logging
   ug_nrf54h20_debugging
   ug_nrf54h20_custom_pcb
   ug_nrf54h20_flpr
   ../nrf54l/zms.rst
