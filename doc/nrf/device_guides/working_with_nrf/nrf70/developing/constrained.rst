.. _nRF70_nRF5340_constrained_host:

Host device considerations
##########################

.. contents::
   :local:
   :depth: 2

This guide provides recommendations and guidelines for using the nRF70 Series device as a companion chip on resource-constrained hosts such as the nRF5340 SoC.

Networking stack combinations
*****************************
The table below lists the required protocol stack combinations for various classes of Wi-Fi® internetworking applications.

.. csv-table:: Supported networking stack combinations
   :file: stack_combo.csv
   :header-rows: 1

Host SoC support
****************
The following table indicates the support of various classes of Wi-Fi networking applications (denoted by the corresponding protocol stack capabilities) running on the different nRF host MCUs combined with the nRF70 Series companion IC.
You can identify and select the appropriate host MCU depending on the IoT use case of interest.

.. csv-table:: Host SoC's support
   :file: soc_combo.csv
   :header-rows: 1

.. note::
   The asterisk * indicates that the SoC is supported, but the use case is not recommended due to performance or memory constraints.

Firmware patch optimizations
****************************
For resource-constrained hosts especially with low internal flash memory, the nRF70 Series firmware patches can significantly contribute to the overall memory footprint.
The nRF70 Series provides the following firmware patch optimizations to reduce the memory footprint of the Wi-Fi host stack:

* A separate firmware patch variant is provided for applications that only require support for Wi-Fi scan-only operation.
  This patch variant is significantly smaller in size compared to the full-featured firmware patch variant that supports Wi-Fi :abbr:`STA (Station)` mode.
* The nRF Wi-Fi driver offers support for either relocating or storing the patch in the external non-volatile memory if that is available.
  See :ref:`ug_nrf70_developing_fw_patch_ext_flash` for details.
