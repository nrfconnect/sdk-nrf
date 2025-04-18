.. _ug_nrf54l:
.. _ug_nrf54L15_gs:

Developing with nRF54L Series
#############################

.. |nrf_series| replace:: devices of the nRF54L Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support and contain board definitions for developing on the nRF54L Series devices:

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
       | `Quick Start app`_
       | `User Guide <nRF54L15 DK User Guide_>`_
     - | `nRF54L15 DK product page <nRF54L15 DK_>`_
       | `nRF54L15 System-on-Chip (SoC) <nRF54L15 System-on-Chip_>`_
   * - nRF54L10 emulation on the nRF54L15 DK
     - PCA10156
     - ``nrf54l15dk/nrf54l10/cpuapp``
     - | `Datasheet <nRF54L15 Datasheet_>`_
     - | `nRF54L10 System-on-Chip (SoC) <nRF54L10_>`_
   * - nRF54L05 emulation on the nRF54L15 DK
     - PCA10156
     - ``nrf54l15dk/nrf54l05/cpuapp``
     - | `Datasheet <nRF54L15 Datasheet_>`_
     - | `nRF54L05 System-on-Chip (SoC) <nRF54L05_>`_

Additionally, the nRF54L15 SoC supports :ref:`nrfxlib:soft_peripherals`, which you can use to emulate the hardware peripherals.

.. _ug_nrf54L15_revision:

Make sure to check the revision of your nRF54L15 development kit to see if it is supported:

.. list-table::
   :header-rows: 1

   * - DK revision
     - Status
   * - nRF54L15 DK v0.9.3
     - Supported
   * - nRF54L15 DK v0.9.2
     - Supported
   * - nRF54L15 DK v0.9.1
     - Supported
   * - nRF54L15 PDK v0.8.1
     - Supported
   * - nRF54L15 PDK v0.7.0 or earlier
     - Deprecated after |NCS| v2.7.0

.. note::

  The supported nRF54L15 DK revisions are functionally equal and use the same board target (``nrf54l15dk/nrf54l15/cpuapp``).

Refer to the compatibility matrices for the nRF54L Series devices to check the compatibility of various SoC revisions with different versions of the |NCS|:

* `nRF54L05 Compatibility Matrix`_
* `nRF54L10 Compatibility Matrix`_
* `nRF54L15 Compatibility Matrix`_

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features
   zms
   cryptography
   vpr_flpr
   building_nrf54l
   nrf54l_signing_with_payload
   fota_update
   kmu_basics
   kmu_provision
