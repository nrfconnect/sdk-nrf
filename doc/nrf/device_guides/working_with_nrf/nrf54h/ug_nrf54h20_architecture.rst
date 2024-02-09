:orphan:

.. _ug_nrf54h20_architecture:

Architecture of nRF54H20
########################

.. contents::
   :local:
   :depth: 2

The nRF54H20 is a multicore System-on-Chip (SoC) that uses an asymmetric multiprocessing (AMP) configuration.
Each core is tasked with specific responsibilities, and is optimized for different workloads.

The following documents introduce the architecture of the nRF54H20 SoC:

* The `Introduction to nRF54H20`_ PDF document, providing an overview of the hardware and software features of the nRF54H20.
* The `nRF54H20 Objective Product Specification 0.3.1`_ (OPS) PDF document.
* The `nRF54H20 prototype difference`_ PDF document, listing the major differences between the final and the prototype silicon provided on the nRF54H20 PDK.

The following pages briefly describe topics like the responsibilities of the cores, their interprocessor interactions, the memory mapping, and the boot sequence in the nRF54H20 SoC.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_architecture_cpu
   ug_nrf54h20_architecture_memory
   ug_nrf54h20_architecture_ipc
   ug_nrf54h20_architecture_boot
   ug_nrf54h20_architecture_lifecycle
