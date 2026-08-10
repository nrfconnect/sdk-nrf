.. _ug_nrf71:
.. _ug_nrf71_developing:

Developing with nRF71 Series
############################

.. |nrf_series| replace:: devices of the nRF71 Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support and contain board definitions for developing on the nRF71 Series devices.
Refer to the following information for the list of supported development kits (DKs) and their related hardware and software documentation:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Board target
     - Documentation
     - Product pages
   * - :zephyr:board:`nrf7120dk`
     - PCA10217
     - | ``nrf7120dk/nrf7120/cpuapp``
       | ``nrf7120dk/nrf7120/cpuapp/ns`` (:ref:`TF-M <app_boards_spe_nspe>`)
       | ``nrf7120dk/nrf7120/cpuflpr``
       | ``nrf7120dk/nrf7120/cpuflpr/xip``
     -
     -

For the full list of supported protocols, see the :ref:`software maturity documentation <software_maturity>`.

.. TODO: Add a link to the Nordic Developer Academy nRF71 Series course once it is available.

The following subpages cover topics related to developing applications on the nRF71 Series devices.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features
   building_nrf71
