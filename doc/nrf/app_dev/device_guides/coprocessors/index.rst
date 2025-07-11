.. _coprocessors_index:

Developing with coprocessors
############################

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

The VPR coprocessor (Fast Lightweight Peripheral Processor - FLPR) can be utilized as follows:

* As an additional core in a multicore system using Zephyr in multithreaded mode.
  See :ref:`vpr_flpr_nrf54l` for more details.
* As a peripheral emulator, using one of the following methods depending on the use case:

  * :ref:`High-Performance Framework (HPF)<hpf_index>`
  * :ref:`nrfxlib:soft_peripherals`, which additionally supports the nRF54H20 DK.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   ug_hpf_softperipherals_comparison.rst
   hpf.rst
