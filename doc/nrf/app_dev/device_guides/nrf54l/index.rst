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
   * - :ref:`zephyr:nrf54l15pdk_nrf54l15`
     - PCA10156
     - | ``nrf54l15pdk@0.2.1/nrf54l15/cpuapp`` for the PDK revision v0.2.1, AB0-ES7 (Engineering A).
       | ``nrf54l15pdk/nrf54l15/cpuapp`` for the PDK revisions v0.3.0 and v0.7.0 (Engineering A).
     - :ref:`Getting started <gsg_other>`
     - `nRF54L15 System-on-Chip`_

.. note::

  The v0.2.1 revision of the nRF54L15 PDK has **Button 3** and **Button 4** connected to GPIO port 2 that do not support interrupts.
  The workaround for this issue is enabled by default with the :kconfig:option:`CONFIG_DK_LIBRARY_BUTTON_NO_ISR` Kconfig option, but it increases the overall power consumption of the system.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features
   testing_dfu
   peripheral_sensor_node_shield
