:orphan:

.. _ug_nrf54h20_architecture:

Architecture of nRF54H20
########################

.. contents::
   :local:
   :depth: 2

The nRF54H20 is a multicore System-on-Chip (SoC) that uses an asymmetric multiprocessing (AMP) configuration.
Each core is tasked with specific responsibilities, and is optimized for different workloads.

The following pages briefly describe topics like the responsibilities of the cores, their interprocessor interactions, the memory mapping, and the boot sequence in nRF54H20.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ug_nrf54h20_architecture_intro
   ug_nrf54h20_architecture_cpu
   ug_nrf54h20_architecture_memory
   ug_nrf54h20_architecture_ipc
   ug_nrf54h20_architecture_boot
   ug_nrf54h20_architecture_lifecycle
