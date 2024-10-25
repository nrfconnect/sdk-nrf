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
     - ``nrf54l15dk/nrf54l15/cpuapp``
     - | `Datasheet <nRF54L15 Datasheet_>`_
       | :ref:`Getting started <gsg_other>`
       | `User Guide <nRF54L15 DK user guide_>`_
     - | `nRF54L15 DK product page <nRF54L15 DK_>`_
       | `nRF54L15 System-on-Chip (SoC) <nRF54L05_>`_
   * - :ref:`nRF54L05 emulation on the nRF54L15 DK <emulated-nrf54l05>`
     - PCA10156
     - ``nrf54l15dk/nrf54l15/cpuapp``
     - | `Datasheet <nRF54L15 Datasheet_>`_
     - | `nRF54L05 System-on-Chip (SoC) <nRF54L15 System-on-Chip_>`_
   * - :ref:`nRF54L10 emulation on the nRF54L15 DK <emulated-nrf54l10>`
     - PCA10156
     - ``nrf54l15dk/nrf54l15/cpuapp``
     - | `Datasheet <nRF54L15 Datasheet_>`_
     - | `nRF54L10 System-on-Chip (SoC) <nRF54L10_>`_

.. _ug_nrf54L15_revision:

Ensure to check the revision of your nRF54L15 device to see if it is supported:

.. list-table::
   :header-rows: 1

   * - DK revision
     - Status
   * - nRF54L15 DK v0.9.1
     - Supported
   * - nRF54L15 PDK v0.8.1
     - Supported
   * - nRF54L15 PDK v0.7.0 or earlier
     - Deprecated after |NCS| v2.7.0

.. note::

  The nRF54L15 DK v0.9.1 and the nRF54L15 PDK v0.8.1 are functionally equal and use the same board target (``nrf54l15dk/nrf54l15/cpuapp``).

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features
   zms
   cryptography
   fota_update
   kmu_provision
   testing_dfu
   vpr_flpr
   building_nrf54l
   snippets
