.. _dect_samples:

DECT NR+ samples
################

This section lists the DECT NR+ full stack and DECT NR+ PHY samples available in the |NCS|.

.. include:: ../samples.rst
    :start-after: samples_general_info_start
    :end-before: samples_general_info_end

|filter_samples_by_board|

The DECT NR+ full stack samples showcase the use of the DECT NR+ full stack with:

* DECT NR+ :ref:`Connection Manager <zephyr:conn_mgr_overview>` and related APIs:

  * .. doxygengroup:: dect_net_l2_mgmt
  * .. doxygengroup:: dect_net_l2

* IPv6 networking capabilities.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: DECT NR+ full stack samples:

   ../../../samples/dect/*/README

The DECT NR+ PHY samples showcase the use of the :ref:`nrf_modem_dect_phy` interface of the :ref:`nrfxlib:nrf_modem`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: DECT NR+ PHY samples:

   ../../../samples/dect/*/*/README
