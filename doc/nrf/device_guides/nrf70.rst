.. _ug_nrf70:

Working with nRF70 Series
#########################

Zephyr and the |NCS| provide support for developing networking applications with Wi-Fi® connectivity using the following nRF70 Series companion IC devices:

.. list-table::
   :header-rows: 1

   * - DK or Prototype platform
     - Companion module
     - PCA number
     - Build target
     - Documentation
   * - nRF7002 DK
     - Not applicable
     - PCA10143
     - ``nrf7002dk/nrf5340/cpuapp``
     - | `Product Specification <Product specification for nRF70 Series devices_>`_
       | :ref:`Getting started <nrf7002dk_nrf5340>`
       | `User Guide <nRF7002 DK Hardware_>`_
   * - nRF7001 emulation on nRF7002 DK
     - Not applicable
     - PCA10143
     - ``nrf7002dk/nrf5340/cpuapp/nrf7001``
     - | `Product Specification <nRF7001 Product Specification_>`_
   * - :ref:`zephyr:nrf5340dk_nrf5340`
     - nRF7002 EK
     - PCA10095
     - ``nrf5340dk/nrf5340/cpuapp``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | :ref:`Getting started <ug_nrf5340_gs>`
       | `User Guide <nRF5340 DK User Guide_>`_
   * - :ref:`zephyr:nrf52840dk_nrf52840`
     - nRF7002 EK
     - PCA10056
     - ``nrf52840dk/nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | :ref:`Getting started <ug_nrf52_gs>`
       | `User Guide <nRF52840 DK User Guide_>`_
   * - :ref:`zephyr:nrf9160dk_nrf9160`
     - nRF7002 EK
     - PCA10090
     - ``nrf9160dk/nrf9160/ns``
     - | `Product Specification <nRF9160 Product Specification_>`_
       | :ref:`Getting started <ug_nrf9160_gs>`
       | `User Guide <nRF9160 DK Hardware_>`_
   * - :ref:`zephyr:nrf9161dk_nrf9161`
     - nRF7002 EK
     - PCA10153
     - ``nrf9161dk/nrf9161/ns``
     - | `Product Specification <nRF9161 Product Specification_>`_
       | `User Guide <nRF9161 DK Hardware_>`_
   * - :ref:`zephyr:thingy53_nrf5340`
     - nRF7002 EB
     - PCA20053
     - ``thingy53/nrf5340/cpuapp``
     - | :ref:`Getting started <ug_thingy53_gs>`
       | `User Guide <Nordic Thingy:53 Hardware_>`_

The following nRF70 Series shields are available and defined in the :file:`nrf/boards/shields` folder:

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Build target
     - Documentation
   * - nRF7002 :term:`Evaluation Kit (EK)`
     - PCA63556
     - ``nrf7002ek``
     - | :ref:`Getting started <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 EK with emulated support for the nRF7001 IC
     - PCA63556
     - ``nrf7002ek_nrf7001``
     - | :ref:`Getting started <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 EK with emulated support for the nRF7000 IC
     - PCA63556
     - ``nrf7002ek_nrf7000``
     - | :ref:`Getting started <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 :term:`Expansion Board (EB)`
     - PCA63561
     - ``nrf7002eb``
     - | :ref:`Getting started <ug_nrf7002eb_gs>`
       | `User Guide <nRF7002 EB User Guide_>`_

Applications can be developed on the nRF7002 DK (PCA10143), which includes the nRF7002 companion IC, or on boards compatible with the nRF7002 EK (PCA63556) or the nRF7002 EB (PCA63561).

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   working_with_nrf/nrf70/features
   working_with_nrf/nrf70/gs
   working_with_nrf/nrf70/nrf7002ek_gs
   working_with_nrf/nrf70/nrf7002eb_gs
   working_with_nrf/nrf70/developing/index
