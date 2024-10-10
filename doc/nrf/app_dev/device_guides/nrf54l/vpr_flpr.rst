.. _vpr_flpr_nrf54l:

Working with the FLPR core
##########################

The nRF54L15 SoC has a dedicated VPR CPU (RISC-V architecture), named *fast lightweight peripheral processor* (FLPR) optimized for software-defined peripherals (SDP).
The following peripherals are available for use with the FLPR core, and can be accessed through the appropriate Zephyr Device Driver API:

* GPIO
* GPIOTE
* GRTC
* TWIM
* UARTE
* VPR

Running FLPR
************

FLPR can function as a generic core, operating under the full Zephyr kernel.
In this configuration, building the FLPR target is similar the application core.
However, the application core build must incorporate an overlay that enables the FLPR coprocessor.
This ensures that the necessary code to initiate FLPR is integrated.

Software Defined Peripherals
----------------------------

FLPR is optimized for implementing SDP.
For more information, see <link tba> documentation page.

Memory allocation
*****************

If a FLPR CPU is running, it can lead to increased latency when accessing ``RAM_01``.
Because of this, when FLPR is used in a project, you should utilize ``RAM_01`` to store only the FLPR code, FLPR data, and the application CPU's non-time-sensitive information.
Conversely, you should use ``RAM_00`` to store data with strict access time requirements such as DMA buffers, and the application CPU data used in low-latency ISRs.
