.. _vpr_flpr_nrf54l:

Working with the FLPR core
##########################

.. contents::
   :local:
   :depth: 2

.. note::
   The FLPR core support in the |NCS| is currently :ref:`experimental<software_maturity>`.
   Additionally, it is not yet available for the nRF54L05 and nRF54L10 SoCs.

The nRF54L15 SoC has a dedicated VPR CPU (RISC-V architecture), named *fast lightweight peripheral processor* (FLPR).
The following peripherals are available for use with the FLPR core, and can be accessed through the appropriate Zephyr Device Driver API:

* GPIO
* GPIOTE
* GRTC
* TWIM
* UARTE
* VPR

.. _vpr_flpr_nrf54l15_initiating:

Using FLPR with Zephyr multithreaded mode
*****************************************

FLPR can function as a generic core, operating under the full Zephyr kernel.
In this configuration, building the FLPR target is similar to the application core.
However, the application core build must incorporate an overlay that enables the FLPR core.

Bootstrapping the FLPR core
===========================

The |NCS| provides a FLPR snippet that adds an overlay required for bootstrapping the FLPR core.
Snippet's primary function is to enable the code that transfers the FLPR code to the designated region (if necessary) and to initiate the FLPR core.

When building for the ``nrf54l15dk/nrf54l15/cpuflpr`` target, a minimal sample is automatically loaded onto the application core.
See more information on :ref:`building_nrf54l_app_flpr_core`.

Peripherals emulation on FLPR
*****************************

.. note::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

The FLPR core can emulate software peripherals using :ref:`nrfxlib:soft_peripherals` or the :ref:`HPF<hpf_index>`.
This setup is useful in scenarios where you need additional peripheral functionality or want to optimize power consumption and performance.
For detailed comparisons of implementation scenarios and use cases between the Soft Peripherals and the HPF, refer to the :ref:`ug_hpf_softperipherals_comparison` documentation page.

MCUboot FLPR configuration
**************************

To ensure that MCUboot functions correctly with a FLPR-integrated application, several manual configurations are necessary.
For details, see :ref:`nRF54l_signing_app_with_flpr_payload`.

Memory allocation
*****************

If a FLPR CPU is running, it can lead to increased latency when accessing ``RAM_01``.
Because of this, when FLPR is used in a project, you should utilize ``RAM_01`` to store only the FLPR code, FLPR data, and the application CPU's non-time-sensitive information.
Conversely, you should use ``RAM_00`` to store data with strict access time requirements such as DMA buffers, and the application CPU data used in low-latency ISRs.
