.. _ug_nrf91:
.. _ug_nrf9160:
.. _ug_nrf9161:
.. _ug_thingy91:
.. _thingy91_ug_intro:


Developing with nRF91 Series
############################

Zephyr and the |NCS| provide support for developing cellular applications using the following nRF91 Series devices:

.. list-table::
   :header-rows: 1

   * - DK or Prototype platform
     - PCA number
     - Board targets
     - Documentation
     - Product pages
   * - :ref:`zephyr:nrf9161dk_nrf9161`
     - PCA10153
     - ``nrf9161dk/nrf9161``, ``nrf9161dk/nrf9161/ns``
     - | `Product Specification <nRF9161 Product Specification_>`_
       | :ref:`Getting started <gsg_guides>`
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
   * - :ref:`zephyr:nrf9151dk_nrf9151`
     - PCA10171
     - ``nrf9151dk/nrf9151``, ``nrf9151dk/nrf9151/ns``
     - | :ref:`Getting started <gsg_guides>`
     - | `nRF9151 System in Package (SiP) <nRF9151 product website_>`_
   * - :ref:`zephyr:nrf9131ek_nrf9131`
     - PCA10165
     - ``nrf9131ek/nrf9131``, ``nrf9131ek/nrf9131/ns``
     - --
     - | `nRF9131 System in Package (SiP) <nRF9131 product website_>`_
   * - Thingy:91
     - PCA20035
     - ``thingy91/nrf9160``, ``thingy91/nrf9160/ns``
     - | :ref:`Getting started <ug_thingy91_gsg>`
       | `User Guide <Nordic Thingy:91 User Guide_>`_
     - | `Thingy\:91 product page`_
       | `nRF9160 System in Package (SiP) <nRF9160 product website_>`_
   * - Thingy:91 X
     - PCA20065
     - ``thingy91x/nrf9151``, ``thingy91x/nrf9151/ns``
     - --
     - | `nRF9151 System in Package (SiP) <nRF9151 product website_>`_

The nRF Connect SDK also offers :ref:`samples <cellular_samples>` dedicated to these devices, as well as compatible :ref:`drivers` and :ref:`libraries`.

If you want to go through a hands-on online training to familiarize yourself with cellular IoT technologies and development of cellular applications, enroll in the `Cellular IoT Fundamentals course`_ in the `Nordic Developer Academy`_.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   nrf91_features
   nrf91_board_controllers
   nrf91_cloud_certificate
   thingy91_connecting
   nrf91_updating_fw_programmer
   nrf91_building
   nrf91_programming
   nrf91_testing_at_client
   nrf91_snippet
   nrf9160_external_flash
