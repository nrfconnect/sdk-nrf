.. _ug_nrf53:

Working with nRF53 Series
#########################

Zephyr and the |NCS| provide support and contain board definitions for developing on the following nRF53 Series devices:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Build target/s
     - Documentation
   * - :ref:`zephyr:nrf5340dk_nrf5340`
     - PCA10095
     - ``nrf5340dk/nrf5340/cpuapp``, ``nrf5340dk/nrf5340/cpuapp/ns``, ``nrf5340dk/nrf5340/cpunet``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | `User Guide <nRF5340 DK User Guide_>`_
   * - :ref:`zephyr:nrf5340_audio_dk_nrf5340`
     - PCA10121
     - ``nrf5340_audio_dk/nrf5340/cpuapp``, ``nrf5340_audio_dk/nrf5340/cpuapp/ns``, ``nrf5340_audio_dk/nrf5340/cpunet``
     - | `Hardware Specification <nRF5340 Audio DK Hardware_>`_
       | :ref:`nrf53_audio_app`
   * - :ref:`zephyr:thingy53_nrf5340`
     - PCA20053
     - ``thingy53/nrf5340/cpuapp``, ``thingy53/nrf5340/cpuapp/ns``, ``thingy53/nrf5340/cpunet``
     - `Hardware Specification <Nordic Thingy:53 Hardware_>`_

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   working_with_nrf/nrf53/features_nrf53
   working_with_nrf/nrf53/nrf5340_gs
   working_with_nrf/nrf53/nrf5340
   working_with_nrf/nrf53/qspi_xip_guide
   working_with_nrf/nrf53/thingy53_gs
   working_with_nrf/nrf53/thingy53
