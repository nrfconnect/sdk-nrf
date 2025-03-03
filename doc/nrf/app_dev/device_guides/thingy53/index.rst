.. _ug_thingy53:

Developing with Thingy:53
#########################

.. |nrf_series| replace:: devices of the nRF53 Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support and contain board definitions for developing on the following nRF53 Series devices:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Board targets
     - Documentation
   * - :ref:`zephyr:thingy53_nrf5340`
     - PCA20053
     - ``thingy53/nrf5340/cpuapp``, ``thingy53/nrf5340/cpuapp/ns``, ``thingy53/nrf5340/cpunet``
     - | `Get started <Nordic Thingy:53 get started_>`_ on the product page
       | `Hardware Specification <Nordic Thingy:53 Hardware_>`_

The Nordic Thingy:53 is a battery-operated prototyping platform for IoT Systems.
The Nordic Thingy:53 integrates the nRF5340 SoC that supports Bluetooth® Low Energy, IEEE 802.15.4 based protocols and Near Field Communication (NFC).
The nRF5340 is augmented with the nRF21540 RF FEM (Front-end Module) Range extender that has an integrated power amplifier (PA)/low-noise amplifier (LNA) and nPM1100 Power Management IC (PMIC) that has an integrated dual-mode buck regulator and battery charger.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   thingy53_precompiled
   building_thingy53
   thingy53_application_guide
