.. _ug_nrf91:

Working with nRF91 Series
#########################

Zephyr and the |NCS| provide support for developing cellular applications using the following nRF91 Series devices:

.. list-table::
   :header-rows: 1

   * - DK or Prototype platform
     - PCA number
     - Build target/s
     - Documentation
     - Product pages
   * - :ref:`zephyr:nrf9161dk_nrf9161`
     - PCA10153
     - ``nrf9161dk/nrf9161``, ``nrf9161dk/nrf9161/ns``
     - | `Product Specification <nRF9161 Product Specification_>`_
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
   * - Thingy91
     - PCA20035
     - ``thingy91/nrf9160``, ``thingy91/nrf9160/ns``
     - | :ref:`Getting started <ug_thingy91_gsg>`
       | `User Guide <Nordic Thingy:91 User Guide_>`_
     - | `Thingy\:91 product page`_
       | `nRF9160 System in Package (SiP) <nRF9160 product website_>`_

The nRF Connect SDK also offers :ref:`samples <cellular_samples>` dedicated to these devices.

To get started with an nRF9161 DK, complete the steps in the Quick Start app in `nRF Connect for Desktop`_.
For more advanced topics related to the nRF9161 DK, see the :ref:`ug_nrf9161` documentation.

If you want to go through a hands-on online training to familiarize yourself with cellular IoT technologies and development of cellular applications, enroll in the `Cellular IoT Fundamentals course`_ in the `Nordic Developer Academy`_.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   working_with_nrf/nrf91/nrf91_features
   working_with_nrf/nrf91/nrf9161
   working_with_nrf/nrf91/nrf9160_gs
   working_with_nrf/nrf91/nrf9160
   working_with_nrf/nrf91/thingy91_gsg
   working_with_nrf/nrf91/thingy91
   working_with_nrf/nrf91/nrf91_snippet
