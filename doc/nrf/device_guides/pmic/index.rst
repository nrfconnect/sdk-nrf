.. _ug_pmic:

Developing with PMICs
#####################

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

.. note::
    Despite being supported in Zephyr, the |NCS| does not support :ref:`zephyr:npm1100_ek` and :ref:`zephyr:npm6001_ek`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   npm1300
