.. _ug_radio_fem:

Developing with Front-End Modules
#################################

Zephyr and the |NCS| provides support for developing applications with the following :term:`Front-End Module (FEM)` devices:

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Board targets
     - Documentation
     - Product pages
   * - :ref:`zephyr:nrf21540dk_nrf52840`
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

Also, various Skyworks front-end modules are supported.
For example, SKY66112-11EK has a 2-pin PA/LNA interface.

The `Front-End Module Feature <nRF21540 Front-End Module_>`_ is supported on the nRF52 and nRF53 Series devices.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   fem_software_support
   fem_power_models
   21540ek_dev_guide
