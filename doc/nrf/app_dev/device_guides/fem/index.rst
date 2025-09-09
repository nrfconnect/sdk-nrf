.. _ug_radio_fem:

Developing with Front-End Modules
#################################

.. |nrf_series| replace:: Front-End Module devices

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provides support for developing applications with the following :term:`Front-End Module (FEM)` devices according to :ref:`software_maturity`:

.. tabs::

   .. group-tab:: nRF52 Series

      .. list-table:: Front-End Module support
         :widths: auto
         :header-rows: 1

         * - FEM device
           - Implementation
           - nRF52833
           - nRF52840
         * - nRF2220
           - nRF2220
           - Supported
           - Supported
         * - nRF21540
           - nRF21540 GPIO
           - Supported
           - Supported
         * - nRF21540
           - nRF21540 GPIO+SPI
           - Supported
           - Supported
         * - SKY66112-11
           - Simple GPIO
           - Supported
           - Supported

   .. group-tab:: nRF53 Series

      .. list-table:: Front-End Module support
         :widths: auto
         :header-rows: 1

         * - FEM device
           - Implementation
           - nRF5340
         * - nRF2220
           - nRF2220
           - Supported
         * - nRF21540
           - nRF21540 GPIO
           - Supported
         * - nRF21540
           - nRF21540 GPIO+SPI
           - Supported
         * - SKY66112-11
           - Simple GPIO
           - Supported

   .. group-tab:: nRF54 Series

      .. list-table:: Front-End Module support
         :widths: auto
         :header-rows: 1

         * - FEM device
           - Implementation
           - nRF54L10
           - nRF54L15
           - nRF54LM20
         * - nRF2220
           - nRF2220
           - Supported
           - Supported
           - Supported
         * - nRF21540
           - nRF21540 GPIO
           - Supported
           - Supported
           - --
         * - nRF21540
           - nRF21540 GPIO+SPI
           - Supported
           - Supported
           - Supported
         * - SKY66112-11
           - Simple GPIO
           - Supported
           - Supported
           - --

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
   * - nRF2220 :term:`Evaluation Kit (EK)`
     - PCA63558
     - ``nrf2220ek``
     -
     -

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   fem_software_support
   fem_power_models
   2220ek_dev_guide
   21540ek_dev_guide
