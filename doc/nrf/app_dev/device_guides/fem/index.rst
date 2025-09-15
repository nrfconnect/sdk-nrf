.. _ug_radio_fem:

Developing with Front-End Modules
#################################

.. |nrf_series| replace:: Front-End Module devices

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provides support for developing applications with the following :term:`Front-End Module (FEM)` devices according to :ref:`software_maturity`:

.. include:: ../../../releases_and_maturity/software_maturity.rst
    :start-after: software_maturity_fem_support_table_start
    :end-before: software_maturity_fem_support_table_end

The FEM support on SoCs that are not listed in the table above might still work, but it is not tested and not guaranteed to work.

The following hardware platforms with :term:`Front-End Module (FEM)` are supported by the |NCS|:

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Board targets
     - Documentation
     - Product pages
   * - :zephyr:board:`nrf21540dk`
     - PCA10112
     - ``nrf21540dk/nrf52840``
     - | `Product Specification <nRF21540 Product Specification_>`_
       | `User Guide <nRF21540 DK User Guide_>`_
     - `nRF21540 DK product page`_

The following FEM :term:`Shield` is available and defined in the :file:`nrf/boards/shields` folder:

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Board target
     - Documentation
     - Product pages
   * - nRF21540 :term:`Evaluation Kit (EK)`
     - PCA63550
     - ``nrf21540ek``
     - | `User Guide <nRF21540 EK User Guide_>`_
       | `Front-End Module Feature <nRF21540 Front-End Module_>`_
     - `nRF21540 DB product page`_

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   fem_software_support
   fem_power_models
   21540ek_dev_guide
