.. _ug_nrf93m1:

Developing with nRF93M1
#######################

.. contents::
   :local:
   :depth: 2

.. |nrf_series| replace:: the nRF93M1 device

.. include:: /includes/guides_complementary_to_app_dev.txt

The nRF93M1 is an LTE Cat 1 bis cellular modem module.
Unlike the :ref:`ug_nrf91`, an nRF93M1 DK does not contain an application core.
The module runs Nordic-maintained modem firmware and exposes its full functionality to a separate host processor over UART or USB.
Your application runs on the host, not on the module.

The host can be any MCU or rich-OS system that can drive a serial link.
On the nRF93M1 DK, the host is an nRF54L15 SoC, which lets you develop the host application in the |NCS| alongside the modem.

The nRF93M1 DK is a development kit that contains the nRF93M1-LABA module, an nRF54L15 host MCU option, and an nRF5340 board controller MCU with SEGGER J-Link support.
It also includes features that enable power consumption measurements and support firmware development for the nRF54-based host MCU.

.. _ug_nrf93m1_supported_boards:

Supported boards
****************

Zephyr and the |NCS| provide support for developing Cat 1 bis applications using the following nRF93M1 device:

.. list-table::
   :header-rows: 1

   * - DK or platform
     - PCA number
     - Board targets
     - Documentation
     - Product pages
   * - :zephyr:board:`nrf93m1dk`
     - PCA10232
     - | ``nrf93m1dk/nrf54l15/cpuapp``
       | ``nrf93m1dk/nrf54l15/cpuapp/ns``
       | ``nrf93m1dk/nrf54l15/cpuflpr``
     - | `nRF93M1 Datasheet`_
       | `nRF93M1 AT Commands Reference Guide`_
       | `nRF93M1 DK Hardware User Guide`_
     - | `nRF93M1 DK product page`_
       | `nRF93M1 module product page`_

The DK is used for development with both module variants described in :ref:`ug_nrf93m1_variants`.

.. note::
   Board revision applies to the DK, for example ``nrf93m1dk@0.3.0/nrf54l15/cpuapp``.
   The modem node is declared in the revision-specific board file :file:`nrf93m1dk_nrf54l15_common_0_3_0.dtsi`.
   Pin mappings differ between revisions, so check the revision of your kit before assuming a pin mapping.

If you want to go through a hands-on online training to familiarize yourself with cellular IoT technologies and development of cellular applications, enroll in the `Cellular IoT Fundamentals course`_ from the `Nordic Developer Academy`_.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   nrf93m1_features
   nrf93m1_interfaces
   nrf93m1_at_commands
   nrf93m1_cloud_connecting
   nrf93m1_building
   nrf93m1_updating_modem_fw
   nrf93m1_power_optimization
   nrf93m1_testing
   nrf93m1_custom_board
