.. _coprocessors_index:

Developing with coprocessors
############################

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

You can use the VPR coprocessor (Fast Lightweight Peripheral Processor - FLPR) as follows:

* As an additional core in a multicore system using Zephyr in multithreaded mode (see the :ref:`nRF54L15<vpr_flpr_nrf54l>` and :ref:`nRF54H20 devices<ug_nrf54h20_flpr>` pages).
* As a peripheral emulator, using one of the following methods depending on the use case:

  * :ref:`High-Performance Framework (HPF)<hpf_index>`
  * :ref:`nrfxlib:soft_peripherals`

.. toctree::
   :maxdepth: 1
   :caption: Contents

   ug_hpf_softperipherals_comparison.rst
   hpf.rst
