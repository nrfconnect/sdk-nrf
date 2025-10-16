.. _vpr_flpr_nrf54l:

Working with the FLPR core
##########################

.. contents::
   :local:
   :depth: 2

The nRF54L series SoCs have a dedicated VPR CPU (RISC-V architecture), named *fast lightweight peripheral processor* (FLPR).

.. _vpr_flpr_nrf54l_support_status:

FLPR support
************

The following section provides a support overview for various FLPR targets:

.. list-table::
   :header-rows: 1

   * - Target
     - Zephyr build target
     - Kernel support
     - Supported drivers
   * - nRF54LM20A FLPR running from SRAM
     - ``nrf54lm20dk/nrf54lm20a/cpuflpr``
     - Experimental
     - --
   * - nRF54LM20A FLPR running from RRAM
     - ``nrf54lm20dk/nrf54lm20a/cpuflpr/xip``
     - Experimental
     - --
   * - nRF54L15 FLPR running from SRAM
     - ``nrf54l15dk/nrf54l15/cpuflpr``
     - Supported
     - * GPIO
       * GPIOTE
       * GRTC
       * TWIM
       * UARTE
       * VPR
   * - nRF54L15 FLPR running from RRAM
     - ``nrf54l15dk/nrf54l15/cpuflpr/xip``
     - Supported
     - --
   * - nRF54L10 FLPR running from SRAM
     - Not available
     - --
     - --
   * - nRF54L10 FLPR running from RRAM
     - Not available
     - --
     - --
   * - nRF54L05 FLPR running from SRAM
     - Not available
     - --
     - --
   * - nRF54L05 FLPR running from RRAM
     - Not available
     - --
     - --

.. _vpr_flpr_nrf54l_initiating:

Using FLPR with Zephyr multithreaded mode
*****************************************

FLPR can function as a generic core, operating under the full Zephyr kernel.
In this configuration, building the FLPR target is similar to the application core.
However, the application core build must incorporate an overlay that enables the FLPR core.

Bootstrapping the FLPR core
===========================

The |NCS| provides an FLPR snippet that adds an overlay required for bootstrapping the FLPR core.
Snippet's primary function is to enable the code that transfers the FLPR code to the designated region (if necessary) and to initiate the FLPR core.

When building for the ``<board target>/cpuflpr`` or ``<board target>/cpuflpr/xip``, where *board target* depends on the device you are using, a minimal sample is automatically loaded onto the application core.
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
