.. _ug_nrf54h:

Working with nRF54H Series
##########################

.. note::

   All software for the nRF54H20 SoC is experimental, and hardware availability is restricted to the participants in the customer sampling program.

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

The following table indicates the compatibility between nRF54H20 firmware bundle versions and |NCS| versions:

.. list-table::
   :header-rows: 1

   * - |NCS| version
     - nRF54H20 firmware bundle version
   * - v2.7
     - v0.5.0
   * - v2.7.99-cs1
     - v0.6.2


.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   working_with_nrf/nrf54h/ug_nrf54h20_gs
   working_with_nrf/nrf54h/ug_nrf54h20_architecture
   working_with_nrf/nrf54h/ug_nrf54h20_configuration
   working_with_nrf/nrf54h/ug_nrf54h20_suit_dfu
   working_with_nrf/nrf54h/ug_nrf54h20_debugging
   working_with_nrf/nrf54h/ug_nrf54h20_custom_pcb
