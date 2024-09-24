.. _ug_nrf54l:
.. _ug_nrf54L15_gs:

Developing with nRF54L Series
#############################

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

.. note::

  When building your project with the nRF54L15 DK v0.8.1 (Engineering B silicon), that is marked as PDK, and the nRF54L15 DK v0.9.1 (Engineering B silicon), you must use the ``nrf54l15dk/nrf54l51/cpuapp`` board target.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features
   cryptography
   testing_dfu
