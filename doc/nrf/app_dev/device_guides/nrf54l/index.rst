.. _ug_nrf54l:
.. _ug_nrf54L15_gs:

Developing with nRF54L Series
#############################

.. note::

  All software for the nRF54L15 SoC is experimental and hardware availability is restricted to the participants in the limited sampling program.

.. |nrf_series| replace:: devices of the nRF54L Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support and contain board definitions for developing on the following nRF54L Series device:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Board target
     - Documentation
     - Product pages
   * - :ref:`zephyr:nrf54l15dk_nrf54l15`
     - PCA10156
     - ``nrf54l15dk/nrf54l51/cpuapp``
     - :ref:`Getting started <gsg_other>`
     - `nRF54L15 System-on-Chip`_

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features
   testing_dfu
   peripheral_sensor_node_shield
