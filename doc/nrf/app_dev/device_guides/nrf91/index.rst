.. _ug_nrf91:
.. _ug_nrf9160:
.. _ug_nrf9161:

Developing with nRF91 Series
############################

.. |nrf_series| replace:: devices of the nRF91 Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support for developing cellular applications using the following nRF91 Series devices:

.. list-table::
   :header-rows: 1

   * - DK or Prototype platform
     - PCA number
     - Board targets
     - Documentation
     - Product pages
   * - :zephyr:board:`nrf9161dk`
     - PCA10153
     - ``nrf9161dk/nrf9161``, ``nrf9161dk/nrf9161/ns``
     - | `Product Specification <nRF9161 Product Specification_>`_
       | `Quick Start app`_
       | `User Guide <nRF9161 DK Hardware_>`_
     - | `nRF9161 DK product page`_
       | `nRF9161 System in Package (SiP) <nRF9161 product website_>`_
   * - :ref:`zephyr:nrf9160dk_nrf9160`
     - PCA10090
     - ``nrf9160dk/nrf9160``, ``nrf9160dk/nrf9160/ns``
     - | `Product Specification <nRF9160 Product Specification_>`_
       | :ref:`Getting started <ug_nrf9160_gs>`
       | `User Guide <nRF9160 DK Hardware_>`_
     - | `nRF9160 DK product page`_
       | `nRF9160 System in Package (SiP) <nRF9160 product website_>`_
   * - :zephyr:board:`nrf9151dk`
     - PCA10171
     - ``nrf9151dk/nrf9151``, ``nrf9151dk/nrf9151/ns``
     - | `Product Specification <nRF9151 Product Specification_>`_
       | `Quick Start app`_
       | `User Guide <nRF9151 DK Hardware_>`_
     - | `nRF9151 DK product page`_
       | `nRF9151 System in Package (SiP) <nRF9151 product website_>`_
   * - :zephyr:board:`nrf9131ek`
     - PCA10165
     - ``nrf9131ek/nrf9131``, ``nrf9131ek/nrf9131/ns``
     - --
     - | `nRF9131 System in Package (SiP) <nRF9131 product website_>`_

The nRF Connect SDK also offers :ref:`Cellular samples <cellular_samples>` dedicated to these devices, :ref:`networking_samples`,  as well as compatible :ref:`drivers` and :ref:`libraries`.

For development of asset tracking applications, it is recommended to base your application on the `Asset Tracker Template <Asset Tracker Template_>`_.

If you want to go through a hands-on online training to familiarize yourself with cellular IoT technologies and development of cellular applications, enroll in the `Cellular IoT Fundamentals course`_ in the `Nordic Developer Academy`_.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   nrf91_features
   nrf91_board_controllers
   nrf91_cloud_connecting
   nrf91_cloud_certificate
   nrf91_dk_updating_fw_programmer
   nrf91_building
   nrf91_programming
   nrf91_testing
   nrf91_snippet
   nrf9160_external_flash
