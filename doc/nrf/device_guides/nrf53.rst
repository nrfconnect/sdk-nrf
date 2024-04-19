.. _ug_nrf53:

Working with nRF53 Series
#########################

Zephyr and the |NCS| provide support and contain board definitions for developing on the following nRF53 Series devices:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Build target
     - Documentation
   * - :ref:`zephyr:nrf5340dk_nrf5340`
     - PCA10095
     - ``nrf5340dk_nrf5340``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | `User Guide <nRF5340 DK User Guide_>`_
   * - :ref:`zephyr:nrf5340_audio_dk_nrf5340`
     - PCA10121
     - ``nrf5340_audio_dk_nrf5340``
     - | `Hardware Specification <nRF5340 Audio DK Hardware_>`_
       | :ref:`nrf53_audio_app`
   * - :ref:`zephyr:thingy53_nrf5340`
     - PCA20053
     - ``thingy53_nrf5340``
     - `Hardware Specification <Nordic Thingy:53 Hardware_>`_

For remote monitoring of fleets running an nRF53 Series SiP, explore :ref:`Memfault <ug_memfault>`.
The nRF Connect SDK includes out-of-the-box metrics collected with Memfault for monitoring Bluetooth connectivity such as connection time and Bluetooth thread stack usage.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   working_with_nrf/nrf53/nrf5340_gs
   working_with_nrf/nrf53/nrf5340
   working_with_nrf/nrf53/qspi_xip_guide
   working_with_nrf/nrf53/thingy53_gs
   working_with_nrf/nrf53/thingy53
