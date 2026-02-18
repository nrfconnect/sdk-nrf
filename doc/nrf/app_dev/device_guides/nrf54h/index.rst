.. _ug_nrf54h:

Developing with nRF54H Series
#############################

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
   :maxdepth: 1
   :caption: Get started with the nRF54H20 DK

   ug_nrf54h20_gs

.. toctree::
   :maxdepth: 1
   :caption: Understand the hardware

   ug_nrf54h20_architecture

.. toctree::
   :maxdepth: 1
   :caption: Configure the nRF54H20 SoC on your custom PCB

   ug_nrf54h20_custom_pcb

.. toctree::
   :maxdepth: 1
   :caption: Develop and debug

   ug_nrf54h20_configuration
   ug_nrf54h20_logging
   ug_nrf54h20_debugging
   ug_nrf54h20_flpr
   ug_nrf54h20_ppr
   ug_nrf54h20_zms

.. toctree::
   :maxdepth: 1
   :caption: Secure your application

   ug_nrf54h20_ironside
   ug_nrf54h20_keys

.. toctree::
   :maxdepth: 1
   :caption: Configure bootloader for Device Firmware Update (DFU)

   ug_nrf54h20_mcuboot_dfu
   ug_nrf54h20_partitioning_merged
   ug_nrf54h20_mcuboot_manifest
   ug_nrf54h20_mcuboot_requests
   ug_nrf54h_ecies_x25519

.. toctree::
   :maxdepth: 1
   :caption: Optimize the power management of your application

   ug_nrf54h20_pm_optimization
