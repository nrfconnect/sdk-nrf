.. _ug_npm1300_features:

Features of nPM1300
###################

.. contents::
    :local:
    :depth: 2

Introduction
************

nPM1300 is a Power Management IC (PMIC) with a linear-mode lithium-ion, lithium-polymer, and lithium ferro-phosphate battery charger.
It has two dual-mode buck regulators, two dual purpose LDO/load switches, three LED drivers and five GPIOs.

nPM1300 is the perfect companion for nRF52 and nRF53 Series SoCs, and the nRF91 series SiP.
It is ideal for compact and advanced IoT products such as wearables and portable medical applications.

For additional information, see the following documentation:

* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.
* `nPM1300`_ for the technical documentation on the nPM1300 PMIC and associated kits.
* :ref:`ug_npm1300_gs`.
* :ref:`ug_npm1300_developing`.

Supported boards
================

nPM1300 is supported by the following boards in the `Zephyr`_ open source project and in |NCS|.

.. list-table::
   :header-rows: 1

   * - DK or Prototype platform
     - Companion module
     - PCA number
     - Build target
     - Documentation
   * - nRF52 DK
     - nPM1300 EK
     - PCA10040
     - ``nrf52dk_nrf52832``
     - | `Product Specification <nRF52832 Product Specification_>`_
       | :ref:`Getting started <ug_nrf52_gs>`
       | `User Guide <nRF52 DK User Guide_>`_
   * - nRF52840 DK
     - nPM1300 EK
     - PCA10056
     - ``nrf52840dk_nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | :ref:`Getting started <ug_nrf52_gs>`
       | `User Guide <nRF52840 DK User Guide_>`_
   * - nRF5340 DK
     - nPM1300 EK
     - PCA10095
     - ``nrf5340dk_nrf5340_cpuapp``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | :ref:`Getting started <ug_nrf5340_gs>`
       | `User Guide <nRF5340 DK User Guide_>`_
   * - nRF9160 DK
     - nPM1300 EK
     - PCA10090
     - ``nrf9160dk_nrf9160_ns``
     - | `Product Specification <nRF9160 Product Specification_>`_
       | :ref:`Getting started <ug_nrf9160_gs>`
       | `User Guide <nRF9160 DK Hardware_>`_
