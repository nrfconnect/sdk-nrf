.. _ug_pmic:

Developing with PMICs
#####################

.. |nrf_series| replace:: Power Management IC devices

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provides support for developing applications with the following Power Management IC devices:

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Board targets
     - Documentation
     - Product pages
   * - :ref:`zephyr:npm1300_ek`
     - PCA10152
     - See :ref:`ug_npm1300_compatible_boards`
     - | `Product Specification <nPM1300 Product Specification_>`_
       | `User Guide <nPM1300 EK User Guide_>`_
       | `nPM1300 EK get started`_
     - | `nPM1300 EK product page`_
       | `nPM1300 Power Management IC (PMIC) <nPM1300 product website_>`_
   * - :ref:`zephyr:npm1304_ek`
     - PCA10195
     - See :ref:`ug_npm1304_compatible_boards`
     - | `Data sheet <nPM1304 Data sheet_>`_
       | `User Guide <nPM1304 EK User Guide_>`_
       | `nPM1304 EK get started`_
     - | `nPM1304 EK product page`_
       | `nPM1304 Power Management IC (PMIC) <nPM1304 product website_>`_
   * - `nPM2100 EK <nPM2100 EK User Guide_>`_
     - PCA10170
     - See :ref:`ug_npm2100_compatible_boards`
     - | `Datasheet <nPM2100 Datasheet_>`_
       | `User Guide <nPM2100 EK User Guide_>`_
     - | `nPM2100 EK product page`_
       | `nPM2100 Power Management IC (PMIC) <nPM2100 product website_>`_

.. note::
    Despite being supported in Zephyr, the |NCS| does not support :ref:`zephyr:npm1100_ek` and :ref:`zephyr:npm6001_ek`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   npm1300
   npm1304
   npm2100
