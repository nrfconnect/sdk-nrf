.. _ug_nrf70:
.. _ug_nrf70_developing:
.. _nrf7002dk_nrf5340:

Developing with nRF70 Series
############################

.. |nrf_series| replace:: devices of the nRF70 Series

.. include:: /includes/guides_complementary_to_app_dev.txt

Zephyr and the |NCS| provide support for developing networking applications with Wi-Fi® connectivity using the following nRF70 Series companion IC devices:

.. list-table::
   :header-rows: 1

   * - DK or Prototype platform
     - Companion module
     - PCA number
     - Board target
     - Documentation
   * - nRF7002 DK
     - Not applicable
     - PCA10143
     - ``nrf7002dk/nrf5340/cpuapp``
     - | `Product Specification <Product specification for nRF70 Series devices_>`_
       | `Quick Start app`_
       | `User Guide <nRF7002 DK Hardware_>`_
   * - nRF7001 emulation on nRF7002 DK
     - Not applicable
     - PCA10143
     - ``nrf7002dk/nrf5340/cpuapp/nrf7001``
     - | `Product Specification <nRF7001 Product Specification_>`_
   * - :zephyr:board:`nrf5340dk`
     - nRF7002 EK
     - PCA10095
     - ``nrf5340dk/nrf5340/cpuapp``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | `Quick Start app`_
       | `User Guide <nRF5340 DK User Guide_>`_
   * - :zephyr:board:`nrf52840dk`
     - nRF7002 EK
     - PCA10056
     - ``nrf52840dk/nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | `Quick Start app`_
       | `User Guide <nRF52840 DK User Guide_>`_
   * - :zephyr:board:`nrf9151dk`
     - nRF7002 EK
     - PCA10171
     - ``nrf9151dk/nrf9151/ns``
     - | `Product Specification <nRF9151 Product Specification_>`_
       | `Quick Start app`_
       | `User Guide <nRF9151 DK Hardware_>`_
   * - :zephyr:board:`nrf9161dk`
     - nRF7002 EK
     - PCA10153
     - ``nrf9161dk/nrf9161/ns``
     - | `Product Specification <nRF9161 Product Specification_>`_
       | `Quick Start app`_
       | `User Guide <nRF9161 DK Hardware_>`_
   * - :ref:`zephyr:nrf9160dk_nrf9160`
     - nRF7002 EK
     - PCA10090
     - ``nrf9160dk/nrf9160/ns``
     - | `Product Specification <nRF9160 Product Specification_>`_
       | :ref:`Getting started <ug_nrf9160_gs>`
       | `User Guide <nRF9160 DK Hardware_>`_
   * - :zephyr:board:`thingy53`
     - nRF7002 EB
     - PCA20053
     - ``thingy53/nrf5340/cpuapp``
     - | :ref:`Getting started <ug_thingy53_gs>`
       | `User Guide <Nordic Thingy:53 Hardware_>`_
   * - :zephyr:board:`nrf54h20dk`
     - nRF7002 EB
     - PCA20053
     - ``nrf54h20dk/nrf54h20/cpuapp``
     - | `nRF54H20 Objective Product Specification 0.3.1`_
       | :ref:`Getting started <ug_nrf54h20_gs>`
   * - :zephyr:board:`nrf54l15dk`
     - nRF7002 EB
     - PCA20053
     - ``nrf54l15dk/nrf54l15/cpuapp``
     - | `Quick Start app`_
       | :ref:`Developing with nRF54L Series <ug_nrf54l15_gs>`


The following nRF70 Series shields are available and defined in the :file:`nrf/boards/shields` folder:

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Board target
     - Documentation
   * - nRF7002 :term:`Evaluation Kit (EK)`
     - PCA63556
     - ``nrf7002ek``
     - | :ref:`Development guide <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 EK with emulated support for the nRF7001 IC
     - PCA63556
     - ``nrf7002ek/nrf7001``
     - | :ref:`Development guide <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 EK with emulated support for the nRF7000 IC
     - PCA63556
     - ``nrf7002ek/nrf7000``
     - | :ref:`Development guide <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 :term:`Expansion Board (EB)` (Deprecated)
     - PCA63561
     - ``nrf7002eb``, ``nrf7002eb_interposer_p1`` (nRF54 Series)
     - | :ref:`Development guide <ug_nrf7002eb_gs>`
       | `User Guide <nRF7002 EB User Guide_>`_
   * - nRF7002-EB II
     - PCA63571
     - ``nrf7002eb2`` (nRF54 Series, supersedes ``nrf7002eb`` for nRF54 Series DKs)
     - | :ref:`Development guide <ug_nrf7002eb2_gs>`

Applications can be developed on the nRF7002 DK (PCA10143), which includes the nRF7002 companion IC, or on boards compatible with the nRF7002 EK (PCA63556) or the nRF7002 EB (PCA63561).

The following subpages cover topics related to developing applications with the nRF70 Series ICs using the respective development and evaluation boards.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   features
   stack_partitioning
   constrained
   fw_patches_ext_flash
   nrf70_fw_patch_update
   power_profiling
   nrf7002ek_dev_guide
   nrf7002eb_dev_guide
   nrf7002eb2_dev_guide
   wifi_advanced_security_modes
