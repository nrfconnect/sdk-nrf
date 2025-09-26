.. _ug_nrf54l:
.. _ug_nrf54L15_gs:
.. _nrf54l_features:
.. _ug_nrf54L15_revision:

Developing with nRF54L Series
#############################

.. |nrf_series| replace:: devices of the nRF54L Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support and contain board definitions for developing on the nRF54L Series devices.
Refer to the following information for the list of supported development kits (DKs) and their related hardware and software documentation:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Board target
     - Documentation
     - Product pages
   * - :zephyr:board:`nrf54lm20dk`
     - PCA10184
     - | ``nrf54lm20dk/nrf54lm20a/cpuapp``
       | ``nrf54lm20dk/nrf54lm20a/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)
       | ``nrf54lm20dk/nrf54lm20a/cpuflpr``
       | ``nrf54lm20dk/nrf54lm20a/cpuflpr/xip``
     - | `Datasheet <nRF54LM20A Datasheet_>`_
       | `Quick Start app`_
       | `User Guide <nRF54LM20 DK User Guide_>`_
       | `nRF54LM20A Compatibility Matrix`_
     - | `nRF54LM20 DK product page <nRF54LM20 DK_>`_
       | `nRF54LM20A System-on-Chip (SoC) <nRF54LM20A System-on-Chip_>`_
   * - :zephyr:board:`nrf54l15dk`
     - PCA10156
     - | ``nrf54l15dk/nrf54l15/cpuapp``
       | ``nrf54l15dk/nrf54l15/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)
       | ``nrf54l15dk/nrf54l15/cpuflpr``
       | ``nrf54l15dk/nrf54l15/cpuflpr/xip``
     - | `Datasheet <nRF54L15 Datasheet_>`_
       | `Quick Start app`_
       | `User Guide <nRF54L15 DK User Guide_>`_
       | `nRF54L15 Compatibility Matrix`_
     - | `nRF54L15 DK product page <nRF54L15 DK_>`_
       | `nRF54L15 System-on-Chip (SoC) <nRF54L15 System-on-Chip_>`_
   * - :ref:`nRF54L10 emulation on the nRF54L15 DK <zephyr:nrf54l15dk_nrf54l10>`
     - PCA10156
     - | ``nrf54l15dk/nrf54l10/cpuapp``
       | ``nrf54l15dk/nrf54l10/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)
     - | `Datasheet <nRF54L15 Datasheet_>`_
       | `nRF54L10 Compatibility Matrix`_
     - | `nRF54L10 System-on-Chip (SoC) <nRF54L10_>`_
   * - :ref:`nRF54L05 emulation on the nRF54L15 DK <zephyr:nrf54l15dk_nrf54l05>`
     - PCA10156
     - ``nrf54l15dk/nrf54l05/cpuapp``
     - | `Datasheet <nRF54L15 Datasheet_>`_
       | `nRF54L05 Compatibility Matrix`_
     - | `nRF54L05 System-on-Chip (SoC) <nRF54L05_>`_

For the full list of supported protocols, see the :ref:`software maturity documentation<software_maturity>`.

Learn the essentials of the nRF54L Series with the `nRF54L Series Express course`_ in the Nordic Developer Academy.
This self-paced course introduces the hardware architecture, functionality, capabilities, and performance of the nRF54L Series.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   zms
   cryptography
   vpr_flpr
   building_nrf54l
   nrf54l_signing_with_payload
   fota_update
   kmu_basics
   kmu_provision
   dfu_config
   ecies_x25519.rst
