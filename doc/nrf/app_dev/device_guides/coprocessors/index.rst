.. _coprocessors_index:

Developing with coprocessors
############################

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

The following pages provide a detailed guide on utilizing coprocessor effectively within the nRF54L15 platform.

The VPR coprocessor (Fast Lightweight Peripheral Processor - FLPR) can be utilized in two distinct ways:

* As an additional core in a multicore system using Zephyr in multithreaded mode.
  See :ref:`vpr_flpr_nrf54l` for more details.
* As a peripheral emulator using the High-Performance Framework (HPF).
  For more information, refer to the following documentation.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   hpf.rst
