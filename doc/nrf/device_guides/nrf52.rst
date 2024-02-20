.. _ug_nrf52_group:
.. _nrf52_supported_boards:

Working with nRF52 Series
#########################

Zephyr and the |NCS| provide support and contain board definitions for developing on the following nRF52 Series devices:

.. list-table::
   :header-rows: 1

   * - DK
     - PCA number
     - Build target
     - Documentation
   * - :ref:`zephyr:nrf52840dk_nrf52840`
     - PCA10056
     - ``nrf52840dk_nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | `User Guide <nRF52840 DK User Guide_>`_
   * - :ref:`zephyr:nrf52840dk_nrf52811`
     - PCA10056
     - ``nrf52840dk_nrf52811``
     - `Product Specification <nRF52811 Product Specification_>`_
   * - :ref:`zephyr:nrf52833dk_nrf52833`
     - PCA10100
     - ``nrf52833dk_nrf52833``
     - | `Product Specification <nRF52833 Product Specification_>`_
       | `User Guide <nRF52833 DK User Guide_>`_
   * - :ref:`zephyr:nrf52833dk_nrf52820`
     - PCA10100
     - ``nrf52833dk_nrf52820``
     - `Product Specification <nRF52820 Product Specification_>`_
   * - :ref:`zephyr:nrf52dk_nrf52832`
     - PCA10040
     - ``nrf52dk_nrf52832``
     - | `Product Specification <nRF52832 Product Specification_>`_
       | `User Guide <nRF52 DK User Guide_>`_
   * - :ref:`zephyr:nrf52dk_nrf52810`
     - PCA10040
     - ``nrf52dk_nrf52810``
     - `Product Specification <nRF52810 Product Specification_>`_
   * - :ref:`zephyr:nrf52dk_nrf52805`
     - PCA10040
     - ``nrf52dk_nrf52805``
     - `Product Specification <nRF52805 Product Specification_>`_
   * - :ref:`zephyr:nrf52840dongle_nrf52840`
     - PCA10059
     - ``nrf52840dongle_nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | `User Guide <nRF52840 Dongle User Guide_>`_
   * - :ref:`zephyr:nrf21540dk_nrf52840`
     - PCA10112
     - ``nrf21540dk_nrf52840``
     - | `Product Specification <nRF21540 Product Specification_>`_

See also :ref:`ug_radio_fem_nrf21540ek` to learn how to use the RF front-end module (FEM) with the nRF52 Series devices.

.. note::
    Despite being supported in :ref:`Zephyr <zephyr:thingy52_nrf52832>`, the |NCS| does not support `Nordic Thingy:52`_.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   working_with_nrf/nrf52/features
   working_with_nrf/nrf52/gs
   working_with_nrf/nrf52/developing
